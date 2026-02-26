#include "PluginProcessor.h"
#include "PluginEditor.h"

// ── Constructor / Destructor ─────────────────────────────────────────────────

SnareMakerAudioProcessor::SnareMakerAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    pBodyFreq    = apvts.getRawParameterValue ("bodyFreq");
    pPitchAmount = apvts.getRawParameterValue ("pitchAmount");
    pPitchDecay   = apvts.getRawParameterValue ("pitchDecay");
    pPitchCurve   = apvts.getRawParameterValue ("pitchCurve");
    pPhaseOffset  = apvts.getRawParameterValue ("phaseOffset");
    pNoiseLevel   = apvts.getRawParameterValue ("noiseLevel");
    pNoiseDecay  = apvts.getRawParameterValue ("noiseDecay");
    pOutputGain  = apvts.getRawParameterValue ("outputGain");
}

SnareMakerAudioProcessor::~SnareMakerAudioProcessor() {}

// ── Parameter layout ─────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout
SnareMakerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    using APF  = juce::AudioParameterFloat;
    using Attr = juce::AudioParameterFloatAttributes;
    using PID  = juce::ParameterID;

    // Body frequency: 50 – 400 Hz, skewed toward low end
    params.push_back (std::make_unique<APF> (
        PID { "bodyFreq", 1 }, "Body Frequency",
        juce::NormalisableRange<float> (50.0f, 400.0f, 0.1f, 0.5f),
        180.0f, Attr{}.withLabel ("Hz")));

    // Pitch sweep amount in semitones above bodyFreq
    params.push_back (std::make_unique<APF> (
        PID { "pitchAmount", 1 }, "Pitch Amount",
        juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f),
        24.0f, Attr{}.withLabel ("st")));

    // Pitch sweep decay time
    params.push_back (std::make_unique<APF> (
        PID { "pitchDecay", 1 }, "Pitch Decay",
        juce::NormalisableRange<float> (1.0f, 500.0f, 0.1f, 0.5f),
        60.0f, Attr{}.withLabel ("ms")));

    // Pitch envelope curve shape  (Phase 2a)
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pitchCurve", 1 }, "Pitch Curve",
        juce::StringArray { "Exponential", "Linear", "Logarithmic" }, 0));

    // Oscillator start phase at trigger  (Phase 2a)
    params.push_back (std::make_unique<APF> (
        PID { "phaseOffset", 1 }, "Phase Offset",
        juce::NormalisableRange<float> (0.0f, 360.0f, 1.0f),
        0.0f, Attr{}.withLabel ("deg")));

    // Noise mix level
    params.push_back (std::make_unique<APF> (
        PID { "noiseLevel", 1 }, "Noise Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.7f));

    // Noise amplitude decay time
    params.push_back (std::make_unique<APF> (
        PID { "noiseDecay", 1 }, "Noise Decay",
        juce::NormalisableRange<float> (1.0f, 500.0f, 0.1f, 0.5f),
        150.0f, Attr{}.withLabel ("ms")));

    // Master output gain
    params.push_back (std::make_unique<APF> (
        PID { "outputGain", 1 }, "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f),
        0.0f, Attr{}.withLabel ("dB")));

    return { params.begin(), params.end() };
}

// ── Playback ─────────────────────────────────────────────────────────────────

void SnareMakerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    isPlaying = false;
    envTime   = 0.0;
    phase     = 0.0;
}

void SnareMakerAudioProcessor::releaseResources() {}

bool SnareMakerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

// ── Process block ─────────────────────────────────────────────────────────────

void SnareMakerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // ── Read parameters (once per block) ──────────────────────────────────
    const float bodyFreq      = pBodyFreq->load();
    const float pitchAmount   = pPitchAmount->load();
    const float pitchDecayMs  = pPitchDecay->load();
    const int   pitchCurveMode = static_cast<int> (pPitchCurve->load());  // Phase 2a
    const float noiseLevel    = pNoiseLevel->load();
    const float noiseDecayMs  = pNoiseDecay->load();
    const float outputGainDb  = pOutputGain->load();

    // Convert ms → sample-count time constants (guard against zero)
    const double sr        = currentSampleRate;
    const double pitchTau  = std::max (1.0, pitchDecayMs * 0.001 * sr);
    const double noiseTau  = std::max (1.0, noiseDecayMs * 0.001 * sr);
    const double bodyTau   = pitchTau * 2.0;   // body amplitude decays slower than pitch sweep
    const double stopTime  = std::max (bodyTau, noiseTau) * 6.0;  // ~-72 dB threshold
    const double gainLin   = juce::Decibels::decibelsToGain (outputGainDb);

    // Pitch start ratio: e.g. pitchAmount=24 → 2 octaves up = ×4
    const double freqRatio = std::pow (2.0, (double) pitchAmount / 12.0);

    // ── Per-sample synthesis ──────────────────────────────────────────────
    auto renderSamples = [&] (int start, int end)
    {
        for (int s = start; s < end; ++s)
        {
            if (!isPlaying) break;

            const double t = envTime;

            // Pitch envelope – shape selected by pitchCurveMode
            // All three modes share pitchTau as the characteristic time constant.
            double pitchEnv;
            switch (pitchCurveMode)
            {
                default:
                case 0:   // Exponential  – fast drop, long tail  (Phase 1 behaviour)
                    pitchEnv = std::exp (-t / pitchTau);
                    break;
                case 1:   // Linear  – constant rate, reaches 0 at 5 × pitchTau
                    pitchEnv = std::max (0.0, 1.0 - t / (pitchTau * 5.0));
                    break;
                case 2:   // Logarithmic (hyperbolic)  – slow fall, long tail
                    pitchEnv = 1.0 / (1.0 + t / pitchTau);
                    break;
            }
            const double currentFreq = bodyFreq * (1.0 + (freqRatio - 1.0) * pitchEnv);

            phase += currentFreq / sr;
            if (phase >= 1.0) phase -= 1.0;

            // Body tone (sine)
            const float bodyOut = (float) (
                std::sin (juce::MathConstants<double>::twoPi * phase)
                * std::exp (-t / bodyTau));

            // Noise component
            const float noiseOut = (random.nextFloat() * 2.0f - 1.0f)
                * (float) (noiseLevel * std::exp (-t / noiseTau));

            const float sample = (bodyOut + noiseOut) * (float) gainLin;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample (ch, s, sample);

            envTime += 1.0;

            if (t >= stopTime)
                isPlaying = false;
        }
    };

    // ── Interleave MIDI events with audio rendering ───────────────────────
    int samplePos = 0;

    for (const auto meta : midiMessages)
    {
        renderSamples (samplePos, meta.samplePosition);
        samplePos = meta.samplePosition;

        const auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            isPlaying = true;
            envTime   = 0.0;
            // Phase 2a: start phase determined by PhaseOffset param (0–360°).
            // phaseOffset=0 → sin(0)=0 → zero-click.  Other angles may add
            // a small onset transient which is intentional for percussion.
            phase = static_cast<double> (pPhaseOffset->load()) / 360.0;
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            isPlaying = false;
        }
        // Note-off is intentionally ignored: the envelope runs to completion.
    }

    renderSamples (samplePos, buffer.getNumSamples());
}

// ── Editor ────────────────────────────────────────────────────────────────────

bool SnareMakerAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SnareMakerAudioProcessor::createEditor()
{
    return new SnareMakerAudioProcessorEditor (*this);
}

// ── Metadata ─────────────────────────────────────────────────────────────────

const juce::String SnareMakerAudioProcessor::getName() const { return JucePlugin_Name; }
bool   SnareMakerAudioProcessor::acceptsMidi()  const { return true;  }
bool   SnareMakerAudioProcessor::producesMidi() const { return false; }
bool   SnareMakerAudioProcessor::isMidiEffect() const { return false; }
double SnareMakerAudioProcessor::getTailLengthSeconds() const { return 1.0; }

int  SnareMakerAudioProcessor::getNumPrograms()                          { return 1;  }
int  SnareMakerAudioProcessor::getCurrentProgram()                       { return 0;  }
void SnareMakerAudioProcessor::setCurrentProgram (int)                   {}
const juce::String SnareMakerAudioProcessor::getProgramName (int)        { return {}; }
void SnareMakerAudioProcessor::changeProgramName (int, const juce::String&) {}

// ── Preset state ─────────────────────────────────────────────────────────────

void SnareMakerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SnareMakerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ── Plugin entry point ───────────────────────────────────────────────────────

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SnareMakerAudioProcessor();
}
