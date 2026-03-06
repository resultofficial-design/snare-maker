#include "PluginProcessor.h"
#include "PluginEditor.h"

// ── Constructor / Destructor ─────────────────────────────────────────────────

SnareMakerAudioProcessor::SnareMakerAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Body oscillator (Phase 1 / 2a)
    pBodyFreq      = apvts.getRawParameterValue ("bodyFreq");
    pPitchAmount   = apvts.getRawParameterValue ("pitchAmount");
    pPitchDecay    = apvts.getRawParameterValue ("pitchDecay");
    pPhaseOffset   = apvts.getRawParameterValue ("phaseOffset");

    // Noise (Phase 1 level + Phase 2b ADSR / filter / brightness)
    pNoiseLevel    = apvts.getRawParameterValue ("noiseLevel");
    pNoiseAttack   = apvts.getRawParameterValue ("noiseAttack");
    pNoiseDecay    = apvts.getRawParameterValue ("noiseDecay");
    pNoiseSustain  = apvts.getRawParameterValue ("noiseSustain");
    pNoiseRelease  = apvts.getRawParameterValue ("noiseRelease");
    pNoiseFiltType = apvts.getRawParameterValue ("noiseFiltType");
    pNoiseFiltFreq = apvts.getRawParameterValue ("noiseFiltFreq");
    pNoiseFiltQ    = apvts.getRawParameterValue ("noiseFiltQ");
    pNoiseBright   = apvts.getRawParameterValue ("noiseBright");

    // Output
    pOutputGain    = apvts.getRawParameterValue ("outputGain");

    // Default 3-point pitch envelope (dynamic N-point)
    // pitchEnvelope uses default ctor: {0,0}, {0.1,1.0}, {1,0} (pitch sweep)

    // Amplitude envelopes: start at 1.0, decay to 0.0
    bodyAmpEnvelope = FlexibleEnvelope ({
        { 0.0f, 1.0f, 0.6f }, { 0.3f, 0.4f, 0.6f }, { 1.0f, 0.0f, 0.0f } });
    noiseAmpEnvelope = FlexibleEnvelope ({
        { 0.0f, 1.0f, 0.6f }, { 0.2f, 0.5f, 0.6f }, { 1.0f, 0.0f, 0.0f } });
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

    // ── Body oscillator (Phase 1 / 2a – unchanged) ────────────────────────

    params.push_back (std::make_unique<APF> (
        PID { "bodyFreq", 1 }, "Body Frequency",
        juce::NormalisableRange<float> (50.0f, 400.0f, 0.1f, 0.5f),
        180.0f, Attr{}.withLabel ("Hz")));

    params.push_back (std::make_unique<APF> (
        PID { "pitchAmount", 1 }, "Pitch Amount",
        juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f),
        24.0f, Attr{}.withLabel ("st")));

    params.push_back (std::make_unique<APF> (
        PID { "pitchDecay", 1 }, "Envelope Time",
        juce::NormalisableRange<float> (1.0f, 500.0f, 0.1f, 0.5f),
        60.0f, Attr{}.withLabel ("ms")));

    params.push_back (std::make_unique<APF> (
        PID { "phaseOffset", 1 }, "Phase Offset",
        juce::NormalisableRange<float> (0.0f, 360.0f, 1.0f),
        0.0f, Attr{}.withLabel ("deg")));

    // ── Noise level (Phase 1) ─────────────────────────────────────────────

    params.push_back (std::make_unique<APF> (
        PID { "noiseLevel", 1 }, "Noise Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.7f));

    // ── Noise ADSR (Phase 2b) ─────────────────────────────────────────────
    // noiseDecay ID is kept from Phase 1 for preset compatibility.

    params.push_back (std::make_unique<APF> (
        PID { "noiseAttack", 1 }, "Noise Attack",
        juce::NormalisableRange<float> (0.1f, 200.0f, 0.1f, 0.5f),
        1.0f, Attr{}.withLabel ("ms")));

    params.push_back (std::make_unique<APF> (
        PID { "noiseDecay", 1 }, "Noise Decay",
        juce::NormalisableRange<float> (1.0f, 500.0f, 0.1f, 0.5f),
        150.0f, Attr{}.withLabel ("ms")));

    params.push_back (std::make_unique<APF> (
        PID { "noiseSustain", 1 }, "Noise Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f));

    params.push_back (std::make_unique<APF> (
        PID { "noiseRelease", 1 }, "Noise Release",
        juce::NormalisableRange<float> (1.0f, 500.0f, 0.1f, 0.5f),
        100.0f, Attr{}.withLabel ("ms")));

    // ── Noise filter (Phase 2b) ───────────────────────────────────────────

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "noiseFiltType", 1 }, "Noise Filter Type",
        juce::StringArray { "High Pass", "Band Pass", "Low Pass" }, 0));

    params.push_back (std::make_unique<APF> (
        PID { "noiseFiltFreq", 1 }, "Noise Filter Freq",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f),
        800.0f, Attr{}.withLabel ("Hz")));

    params.push_back (std::make_unique<APF> (
        PID { "noiseFiltQ", 1 }, "Noise Filter Q",
        juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f),
        0.707f));

    // ── Noise brightness / tilt EQ (Phase 2b) ────────────────────────────

    params.push_back (std::make_unique<APF> (
        PID { "noiseBright", 1 }, "Noise Brightness",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f),
        0.0f, Attr{}.withLabel ("dB")));

    // ── Master output ─────────────────────────────────────────────────────

    params.push_back (std::make_unique<APF> (
        PID { "outputGain", 1 }, "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f),
        0.0f, Attr{}.withLabel ("dB")));

    return { params.begin(), params.end() };
}

// =============================================================================
// Private helpers (Phase 2c)
// =============================================================================

// ── resetVoiceState ───────────────────────────────────────────────────────────
// Resets ALL synthesis state to silent/idle.  Called from prepareToPlay()
// and killAll().  Safe to call from the audio thread.

void SnareMakerAudioProcessor::resetVoiceState()
{
    // Body oscillator
    isPlaying      = false;
    envTime        = 0.0;
    phase          = 0.0;

    // Pitch envelope hard-stop
    pitchEnvelope.reset();

    // Body declick
    bodyLastOutput = 0.0f;
    bodyFadeSample = 0.0f;
    bodyFadeLeft   = 0;
    bodyOnsetGain  = 1.0;
    bodyOnsetRate  = 0.0;

    // Noise ADSR
    noiseStage     = NoiseStage::Idle;
    noiseEnvLevel  = 0.0;

    // Filter delay registers (avoid stale state producing noise on next note)
    noiseTypeFilter.reset();
    noiseBrightFilter.reset();
}

// ── triggerNote ───────────────────────────────────────────────────────────────
// Starts a new body + noise voice.
//
// Zero-click design:
//   Body (first trigger):  phaseOffset=0 → sin(0)=0 → output starts at ~0.
//   Body (retrigger):      bodyOnsetGain ramps 0→1 while bodyFadeSample fades
//                          1→0.  Combined output is continuous at the boundary.
//   Noise (any):           Attack always ramps from current noiseEnvLevel, so
//                          no discontinuity regardless of retrigger state.

void SnareMakerAudioProcessor::triggerNote (float phaseOffsetDeg)
{
    if (isPlaying)
    {
        // ── Retrigger declick ─────────────────────────────────────────────
        // Freeze the last body output value and schedule a linear fade-out.
        // Simultaneously ramp the new body from 0 to avoid double-amplitude.
        bodyFadeSample = bodyLastOutput;
        bodyFadeLeft   = kDeclickSamples;
        bodyOnsetGain  = 0.0;
        bodyOnsetRate  = 1.0 / (double) kDeclickSamples;
    }
    else
    {
        // ── Fresh trigger ─────────────────────────────────────────────────
        // No fade needed: previous body was already silent.
        // bodyOnsetGain=1 so no ramping overhead on first note.
        bodyFadeLeft  = 0;
        bodyOnsetGain = 1.0;
        bodyOnsetRate = 0.0;
    }

    // Reset body oscillator
    isPlaying = true;
    envTime   = 0.0;
    phase     = static_cast<double> (phaseOffsetDeg) / 360.0;

    // Start pitch envelope from the beginning
    pitchEnvelope.noteOn();

    // Retrigger noise ADSR from current level (zero-click: linear attack ramps
    // from wherever noiseEnvLevel currently is, not forced to 0).
    noiseStage = NoiseStage::Attack;
}

// ── releaseNote ───────────────────────────────────────────────────────────────
// Responds to MIDI note-off.
//   Body:  intentionally runs to natural completion (percussive design).
//   Noise: enters Release stage so the tail decays cleanly.

void SnareMakerAudioProcessor::releaseNote()
{
    // Pitch envelope: no-op (percussive, runs to natural completion)
    pitchEnvelope.noteOff();

    if (noiseStage == NoiseStage::Attack
     || noiseStage == NoiseStage::Decay
     || noiseStage == NoiseStage::Sustain)
    {
        noiseStage = NoiseStage::Release;
    }
}

// ── killAll ───────────────────────────────────────────────────────────────────
// Hard-stops all audio immediately (AllNotesOff / AllSoundOff).
// A small transient click is acceptable and expected for an emergency stop.

void SnareMakerAudioProcessor::killAll()
{
    resetVoiceState();
}

// =============================================================================
// Playback
// =============================================================================

void SnareMakerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    pitchEnvelope.prepare (sampleRate);
    resetVoiceState();

    // Initialise output gain smoother.
    // 10 ms linear ramp eliminates zipper noise for real-time knob moves and
    // DAW automation.  setCurrentAndTargetValue() snaps to the current preset
    // value so the first block starts at the right level with no ramp.
    gainSmooth.reset (sampleRate, 0.010);
    gainSmooth.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (pOutputGain->load()));
}

void SnareMakerAudioProcessor::releaseResources() {}

bool SnareMakerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
}

// =============================================================================
// Process block
// =============================================================================

void SnareMakerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // ── Read parameters once per block ────────────────────────────────────
    // All reads are from std::atomic<float>*, safe on the audio thread.

    const float bodyFreq        = pBodyFreq->load();
    const float pitchAmount     = pPitchAmount->load();
    const float pitchDecayMs    = pPitchDecay->load();
    const float phaseOffsetDeg  = pPhaseOffset->load();

    // Lock envelope for the duration of this block (UI may add/remove points)
    juce::SpinLock::ScopedLockType envLock (envelopeLock);
    pitchEnvelope.setDuration ((double) pitchDecayMs);

    const float noiseLevel      = pNoiseLevel->load();
    const float noiseAttackMs   = pNoiseAttack->load();
    const float noiseDecayMs    = pNoiseDecay->load();
    const float noiseSustainLvl = pNoiseSustain->load();
    const float noiseReleaseMs  = pNoiseRelease->load();
    const int   noiseFiltMode   = static_cast<int> (pNoiseFiltType->load());
    const float noiseFiltFreq   = pNoiseFiltFreq->load();
    const float noiseFiltQ      = pNoiseFiltQ->load();
    const float noiseBrightDb   = pNoiseBright->load();

    const float outputGainDb    = pOutputGain->load();

    // ── Derived time constants ─────────────────────────────────────────────

    const double sr = currentSampleRate;

    // Body amplitude decay: tau = 2× envelope duration, stop at ≈ -72 dB.
    // (Pitch envelope duration is managed by pitchEnvelope.setDuration above.)
    const double bodyTau      = std::max (1.0, pitchDecayMs * 0.001 * sr) * 2.0;
    const double bodyStopTime = bodyTau * 6.0;

    // Noise ADSR (all in samples)
    const double attackSamples  = std::max (1.0, noiseAttackMs  * 0.001 * sr);
    const double decaySamples   = std::max (1.0, noiseDecayMs   * 0.001 * sr);
    const double releaseSamples = std::max (1.0, noiseReleaseMs * 0.001 * sr);

    // One-pole exponential-approach coefficients.
    // After N samples, |level − target| ≈ initial_distance × e^-1.
    const double decayCoeff   = 1.0 - std::exp (-1.0 / decaySamples);
    const double releaseCoeff = 1.0 - std::exp (-1.0 / releaseSamples);

    // Pitch ratio: semitones above bodyFreq at trigger
    const double freqRatio = std::pow (2.0, (double) pitchAmount / 12.0);

    // ── Update noise filter coefficients once per block ────────────────────
    // Recomputing every block is safe and cheap; avoids per-sample branching.

    switch (noiseFiltMode)
    {
        case 0:  noiseTypeFilter.setHighPass (sr, noiseFiltFreq, noiseFiltQ); break;
        case 1:  noiseTypeFilter.setBandPass (sr, noiseFiltFreq, noiseFiltQ); break;
        case 2:  noiseTypeFilter.setLowPass  (sr, noiseFiltFreq, noiseFiltQ); break;
        default: noiseTypeFilter.setHighPass (sr, noiseFiltFreq, noiseFiltQ); break;
    }

    // High shelf at 3 kHz; Q=0.707 gives a maximally-flat shelf transition.
    noiseBrightFilter.setHighShelf (sr, 3000.0, 0.707, noiseBrightDb);

    // ── Arm gain smoother for this block ──────────────────────────────────
    // setTargetValue() queues the ramp; getNextValue() is called per-sample
    // below so the smoother always stays exactly in sync with buffer position.
    gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (outputGainDb));

    // ── Per-sample synthesis ───────────────────────────────────────────────

    auto renderSamples = [&] (int start, int end)
    {
        for (int s = start; s < end; ++s)
        {
            // ── Advance gain smoother for EVERY sample ─────────────────────
            // Must happen unconditionally so the smoother stays in sync
            // with the host's buffer position even during silent blocks.
            const float gainThisSample = gainSmooth.getNextValue();

            const bool bodyActive    = isPlaying;
            const bool noiseActive   = (noiseStage != NoiseStage::Idle);
            const bool declickActive = (bodyFadeLeft > 0);

            // Nothing to render – skip sample but keep smoother advanced.
            if (!bodyActive && !noiseActive && !declickActive)
                continue;

            // ── Body oscillator ────────────────────────────────────────────
            float bodyOut = 0.0f;

            if (bodyActive)
            {
                const double t = envTime;
                envTime += 1.0;

                // Pitch envelope: stateful per-sample advance
                const double pitchEnv = pitchEnvelope.getNextSample();

                const double currentFreq = bodyFreq * (1.0 + (freqRatio - 1.0) * pitchEnv);

                phase += currentFreq / sr;
                if (phase >= 1.0) phase -= 1.0;

                const float rawBody = (float) (
                    std::sin (juce::MathConstants<double>::twoPi * phase)
                    * std::exp (-t / bodyTau));

                // Onset gain ramp (zero-click on retrigger): ramps 0→1 over
                // kDeclickSamples, then stays at 1.0 for the rest of the note.
                if (bodyOnsetGain < 1.0)
                    bodyOnsetGain = std::min (1.0, bodyOnsetGain + bodyOnsetRate);

                bodyOut        = rawBody * (float) bodyOnsetGain;
                bodyLastOutput = bodyOut;  // snapshot for next retrigger declick

                // Stop body when amplitude is negligible (≈ −72 dB).
                // Update isPlaying before the Sustain→Release check below.
                if (t >= bodyStopTime)
                    isPlaying = false;
            }

            // ── Body declick tail (Phase 2c) ───────────────────────────────
            // Fades out the frozen last-output of the previous voice over
            // kDeclickSamples, crossfading with the new body's onset ramp.
            if (declickActive)
            {
                const float fadeGain = (float) bodyFadeLeft / (float) kDeclickSamples;
                bodyOut += bodyFadeSample * fadeGain;
                --bodyFadeLeft;
            }

            // ── Noise ADSR (Phase 2b) ──────────────────────────────────────
            float noiseOut = 0.0f;

            if (noiseActive)
            {
                switch (noiseStage)
                {
                    case NoiseStage::Attack:
                        // Linear ramp: zero-click because we start from the
                        // current noiseEnvLevel (may be non-zero on retrigger).
                        noiseEnvLevel += 1.0 / attackSamples;
                        if (noiseEnvLevel >= 1.0)
                        {
                            noiseEnvLevel = 1.0;
                            noiseStage    = NoiseStage::Decay;
                        }
                        break;

                    case NoiseStage::Decay:
                        // Exponential approach toward sustain level.
                        noiseEnvLevel += ((double) noiseSustainLvl - noiseEnvLevel)
                                         * decayCoeff;
                        if (noiseEnvLevel <= (double) noiseSustainLvl + 1e-4)
                        {
                            noiseEnvLevel = noiseSustainLvl;
                            // sustain=0 → skip Sustain, go straight to Idle.
                            noiseStage = (noiseSustainLvl > 1e-4)
                                         ? NoiseStage::Sustain
                                         : NoiseStage::Idle;
                        }
                        break;

                    case NoiseStage::Sustain:
                        // Hold at sustain level.
                        // Auto-release when the body oscillator finishes so
                        // the sound never sustains indefinitely without note-off.
                        if (!isPlaying)
                            noiseStage = NoiseStage::Release;
                        break;

                    case NoiseStage::Release:
                        // Exponential decay toward 0.
                        noiseEnvLevel -= noiseEnvLevel * releaseCoeff;
                        if (noiseEnvLevel < 1e-5)
                        {
                            noiseEnvLevel = 0.0;
                            noiseStage    = NoiseStage::Idle;
                        }
                        break;

                    case NoiseStage::Idle:
                        break;
                }

                // White noise → type filter (HP / BP / LP) → brightness shelf
                const float rawNoise = (random.nextFloat() * 2.0f - 1.0f)
                                       * noiseLevel * (float) noiseEnvLevel;
                noiseOut = noiseBrightFilter.process (noiseTypeFilter.process (rawNoise));
            }

            // ── Mix and apply smoothed output gain ─────────────────────────
            const float sample = (bodyOut + noiseOut) * gainThisSample;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample (ch, s, sample);
        }
    };

    // ── Interleave MIDI events with audio rendering (sample-accurate) ──────

    int samplePos = 0;

    for (const auto meta : midiMessages)
    {
        renderSamples (samplePos, meta.samplePosition);
        samplePos = meta.samplePosition;

        const auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            triggerNote (phaseOffsetDeg);
        }
        else if (msg.isNoteOff())
        {
            releaseNote();
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            killAll();
        }
    }

    renderSamples (samplePos, buffer.getNumSamples());
}

// =============================================================================
// Editor
// =============================================================================

bool SnareMakerAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* SnareMakerAudioProcessor::createEditor()
{
    return new SnareMakerAudioProcessorEditor (*this);
}

// =============================================================================
// Metadata
// =============================================================================

const juce::String SnareMakerAudioProcessor::getName() const { return JucePlugin_Name; }
bool   SnareMakerAudioProcessor::acceptsMidi()  const { return true;  }
bool   SnareMakerAudioProcessor::producesMidi() const { return false; }
bool   SnareMakerAudioProcessor::isMidiEffect() const { return false; }

double SnareMakerAudioProcessor::getTailLengthSeconds() const
{
    // Body amplitude decays to −72 dB threshold in 6 × bodyTau = 12 × pitchDecay.
    // Noise Release reaches our 1e-5 threshold in ≈ 12 × noiseRelease (for the
    // exponential decay used in the Release stage).
    // Return whichever tail is longer so the DAW doesn't cut the bounce early.
    const double pitchDecaySec   = (double) pPitchDecay->load()  * 0.001;
    const double noiseReleaseSec = (double) pNoiseRelease->load() * 0.001;
    return std::max (pitchDecaySec * 12.0, noiseReleaseSec * 12.0);
}

// =============================================================================
// Programs
// =============================================================================

int  SnareMakerAudioProcessor::getNumPrograms()                             { return 1;  }
int  SnareMakerAudioProcessor::getCurrentProgram()                          { return 0;  }
void SnareMakerAudioProcessor::setCurrentProgram (int)                      {}
const juce::String SnareMakerAudioProcessor::getProgramName (int)           { return {}; }
void SnareMakerAudioProcessor::changeProgramName (int, const juce::String&) {}

// =============================================================================
// Preset state
// =============================================================================

void SnareMakerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        auto saveEnvelope = [&] (const char* tag, const FlexibleEnvelope& env)
        {
            auto* el = xml->createNewChildElement (tag);
            for (const auto& pt : env.points)
            {
                auto* ptXml = el->createNewChildElement ("Point");
                ptXml->setAttribute ("time",  (double) pt.time);
                ptXml->setAttribute ("value", (double) pt.value);
                ptXml->setAttribute ("curve", (double) pt.curve);
            }
        };

        {
            juce::SpinLock::ScopedLockType lock (envelopeLock);
            saveEnvelope ("PitchEnvelope",    pitchEnvelope);
            saveEnvelope ("BodyAmpEnvelope",  bodyAmpEnvelope);
            saveEnvelope ("NoiseAmpEnvelope", noiseAmpEnvelope);
            saveEnvelope ("RoomAmpEnvelope",  roomAmpEnvelope);
        }
        copyXmlToBinary (*xml, destData);
    }
}

void SnareMakerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

        auto loadEnvelope = [&] (const char* tag, FlexibleEnvelope& env)
        {
            if (auto* el = xml->getChildByName (tag))
            {
                std::vector<EnvelopePoint> pts;
                for (auto* ptXml : el->getChildIterator())
                    if (ptXml->hasTagName ("Point"))
                        pts.push_back ({
                            (float) ptXml->getDoubleAttribute ("time",  0.0),
                            (float) ptXml->getDoubleAttribute ("value", 0.0),
                            (float) ptXml->getDoubleAttribute ("curve", 0.0) });

                if (pts.size() >= 2)
                    env.points = std::move (pts);
            }
        };

        juce::SpinLock::ScopedLockType lock (envelopeLock);
        loadEnvelope ("PitchEnvelope",    pitchEnvelope);
        loadEnvelope ("BodyAmpEnvelope",  bodyAmpEnvelope);
        loadEnvelope ("NoiseAmpEnvelope", noiseAmpEnvelope);
        loadEnvelope ("RoomAmpEnvelope",  roomAmpEnvelope);
    }
}

// =============================================================================
// Plugin entry point
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SnareMakerAudioProcessor();
}
