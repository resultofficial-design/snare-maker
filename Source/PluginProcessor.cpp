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

    // Per-layer volume (Phase 9)
    pTransientLevel = apvts.getRawParameterValue ("transientLevel");
    pBodyLevel      = apvts.getRawParameterValue ("bodyLevel");
    pResonantLevel  = apvts.getRawParameterValue ("resonantLevel");
    pRoomLevel      = apvts.getRawParameterValue ("roomLevel");

    // Per-layer width (Phase 10)
    pTransientWidth = apvts.getRawParameterValue ("transientWidth");
    pBodyWidth      = apvts.getRawParameterValue ("bodyWidth");
    pResonantWidth  = apvts.getRawParameterValue ("resonantWidth");
    pNoiseWidth     = apvts.getRawParameterValue ("noiseWidth");
    pRoomWidth      = apvts.getRawParameterValue ("roomWidth");

    // Output
    pOutputGain    = apvts.getRawParameterValue ("outputGain");

    // Register audio formats for sample loading (wav, aiff, etc.)
    formatManager.registerBasicFormats();

    // Default 3-point pitch envelope (dynamic N-point)
    // pitchEnvelope uses default ctor: {0,0}, {0.1,1.0}, {1,0} (pitch sweep)

    // Amplitude envelopes: start at 1.0, decay to 0.0
    transientAmpEnvelope = FlexibleEnvelope ({
        { 0.0f, 1.0f, 0.0f }, { 0.05f, 0.6f, 0.0f }, { 1.0f, 0.0f, 0.0f } });
    bodyAmpEnvelope = FlexibleEnvelope ({
        { 0.0f, 1.0f, 0.0f }, { 0.3f, 0.4f, 0.0f }, { 1.0f, 0.0f, 0.0f } });
    noiseAmpEnvelope = FlexibleEnvelope ({
        { 0.0f, 1.0f, 0.0f }, { 0.2f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } });
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

    // ── Per-layer volume knobs (Phase 9) ──────────────────────────────────

    params.push_back (std::make_unique<APF> (
        PID { "transientLevel", 1 }, "Transient Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<APF> (
        PID { "bodyLevel", 1 }, "Body Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<APF> (
        PID { "resonantLevel", 1 }, "Resonant Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<APF> (
        PID { "roomLevel", 1 }, "Room Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    // ── Per-layer width (Phase 10) ─────────────────────────────────────────

    params.push_back (std::make_unique<APF> (
        PID { "transientWidth", 1 }, "Transient Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<APF> (
        PID { "bodyWidth", 1 }, "Body Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<APF> (
        PID { "resonantWidth", 1 }, "Resonant Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<APF> (
        PID { "noiseWidth", 1 }, "Noise Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    params.push_back (std::make_unique<APF> (
        PID { "roomWidth", 1 }, "Room Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f));

    // ── Master output ─────────────────────────────────────────────────────

    params.push_back (std::make_unique<APF> (
        PID { "outputGain", 1 }, "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f),
        0.0f, Attr{}.withLabel ("dB")));

    return { params.begin(), params.end() };
}

// =============================================================================
// Sample loading (Phase 8)
// =============================================================================

bool SnareMakerAudioProcessor::loadSampleFromFile (const juce::String& filePath,
                                                    juce::AudioBuffer<float>& dest,
                                                    juce::String& destPath)
{
    juce::File file (filePath);
    if (! file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return false;

    const int numSamples = (int) reader->lengthInSamples;
    const int numChannels = (int) reader->numChannels;

    juce::AudioBuffer<float> tempBuffer (numChannels, numSamples);
    reader->read (&tempBuffer, 0, numSamples, 0, true, true);

    // Mono mixdown if stereo
    dest.setSize (1, numSamples);
    if (numChannels == 1)
    {
        dest.copyFrom (0, 0, tempBuffer, 0, 0, numSamples);
    }
    else
    {
        dest.copyFrom (0, 0, tempBuffer, 0, 0, numSamples);
        dest.addFrom  (0, 0, tempBuffer, 1, 0, numSamples);
        dest.applyGain (0.5f);
    }

    destPath = filePath;
    return true;
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

    // Sample playback
    transientPlayPos   = 0;
    resonantPlayPos    = 0;
    noiseSamplePlayPos = 0;
    transientPlaying   = false;
    resonantPlaying    = false;
    noiseSamplePlaying = false;

    // Playback position for UI
    playbackPositionSec.store (-1.0f, std::memory_order_relaxed);

    // Filter delay registers (avoid stale state producing noise on next note)
    noiseTypeFilter.reset();
    noiseBrightFilter.reset();

    // Width processing state
    for (int i = 0; i < kWidthLayers; ++i)
        widthStates[i].reset();
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

    // Start sample playback for loaded layers
    if (transientSampleBuffer.getNumSamples() > 0)
    {
        transientPlaying = true;
        transientPlayPos = 0;
    }
    if (resonantSampleBuffer.getNumSamples() > 0)
    {
        resonantPlaying = true;
        resonantPlayPos = 0;
    }
    if (noiseSampleBuffer.getNumSamples() > 0)
    {
        noiseSamplePlaying = true;
        noiseSamplePlayPos = 0;
    }
}

// ── releaseNote ───────────────────────────────────────────────────────────────
// Responds to MIDI note-off.
//   Body:  intentionally runs to natural completion (percussive design).
//   Noise: enters Release stage so the tail decays cleanly.

void SnareMakerAudioProcessor::releaseNote()
{
    // Percussive: all envelopes run to natural completion, note-off is a no-op.
    pitchEnvelope.noteOff();
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

    // Width crossover filters: Butterworth LP+HP at crossover frequency
    for (int i = 0; i < kWidthLayers; ++i)
    {
        widthStates[i].lpFilter.setLowPass  (sampleRate, kCrossoverFreq, 0.707);
        widthStates[i].hpFilter.setHighPass (sampleRate, kCrossoverFreq, 0.707);
    }

    // Decorrelation delay: ~0.3 ms, clamped to buffer size
    decoDelaySamples = juce::jlimit (1, kDecoDelayMax - 1,
                                      (int) (sampleRate * 0.0003));
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
    const int   noiseFiltMode   = static_cast<int> (pNoiseFiltType->load());
    const float noiseFiltFreq   = pNoiseFiltFreq->load();
    const float noiseFiltQ      = pNoiseFiltQ->load();
    const float noiseBrightDb   = pNoiseBright->load();

    // Per-layer volume knobs (Phase 9)
    const float transientLevel  = pTransientLevel->load();
    const float bodyLevel       = pBodyLevel->load();
    const float resonantLevel   = pResonantLevel->load();
    const float roomLevel       = pRoomLevel->load();
    (void) roomLevel;   // Room audio path not yet implemented; parameter ready for future use

    // Per-layer width (Phase 10) — 0=mono, 0.5=natural, 1.0=wide
    const float transientWidth  = pTransientWidth->load();
    const float bodyWidth       = pBodyWidth->load();
    const float resonantWidth   = pResonantWidth->load();
    const float noiseWidth      = pNoiseWidth->load();
    const float roomWidth       = pRoomWidth->load();
    (void) roomWidth;

    const float outputGainDb    = pOutputGain->load();

    // ── Derived time constants ─────────────────────────────────────────────

    const double sr = currentSampleRate;

    // Pitch ratio: semitones above bodyFreq at trigger
    const double freqRatio = std::pow (2.0, (double) pitchAmount / 12.0);

    // Voice duration: used to normalise amp envelope lookups.
    // 3× pitchDecay so the envelope spans a longer playback window.
    const double bodyEnvDurSec = std::max (0.001, (double) pitchDecayMs * 0.001) * 3.0;
    const double bodyEnvDurSamples = bodyEnvDurSec * sr;

    // Noise uses the same voice duration for its amp envelope normalisation.
    // Noise ADSR params (attack/decay/sustain/release) are no longer used for
    // amplitude shaping — the amp envelope handles it.  Filters still apply.
    const double noiseEnvDurSamples = bodyEnvDurSamples;

    // Voice stop time: when envTime exceeds this AND all samples are done.
    // Use a generous multiplier so the amp envelope tail isn't cut short.
    const double voiceStopTime = bodyEnvDurSamples * 2.0;

    // ── Update noise filter coefficients once per block ────────────────────

    switch (noiseFiltMode)
    {
        case 0:  noiseTypeFilter.setHighPass (sr, noiseFiltFreq, noiseFiltQ); break;
        case 1:  noiseTypeFilter.setBandPass (sr, noiseFiltFreq, noiseFiltQ); break;
        case 2:  noiseTypeFilter.setLowPass  (sr, noiseFiltFreq, noiseFiltQ); break;
        default: noiseTypeFilter.setHighPass (sr, noiseFiltFreq, noiseFiltQ); break;
    }

    noiseBrightFilter.setHighShelf (sr, 3000.0, 0.707, noiseBrightDb);

    // ── Arm gain smoother for this block ──────────────────────────────────
    gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (outputGainDb));

    // ── Layer enabled flags (read once per block) ────────────────────────
    const bool layerTransient = transientEnabled.load (std::memory_order_relaxed);
    const bool layerBody      = bodyEnabled.load      (std::memory_order_relaxed);
    const bool layerResonant  = resonantEnabled.load  (std::memory_order_relaxed);
    const bool layerNoise     = noiseEnabled.load     (std::memory_order_relaxed);

    // ── Sample buffer lengths (read once per block, safe: samples are mono) ─
    const int transientLen = transientSampleBuffer.getNumSamples();
    const int resonantLen  = resonantSampleBuffer.getNumSamples();
    const int noiseSmpLen  = noiseSampleBuffer.getNumSamples();

    const float* transientData = (transientLen > 0) ? transientSampleBuffer.getReadPointer (0) : nullptr;
    const float* resonantData  = (resonantLen  > 0) ? resonantSampleBuffer.getReadPointer (0)  : nullptr;
    const float* noiseSmpData  = (noiseSmpLen  > 0) ? noiseSampleBuffer.getReadPointer (0)     : nullptr;

    // ── Stereo width helper ──────────────────────────────────────────────
    // Applies phase-safe stereo widening: crossover splits low (mono) from
    // high band; decorrelation delay creates stereo content from mono;
    // mid/side scaling controls width.  width 0=mono, 0.5=natural, 1=wide.
    const int numCh = buffer.getNumChannels();
    const int decoD = decoDelaySamples;

    auto applyWidth = [&] (float mono, float width, WidthState& ws,
                           float& outL, float& outR)
    {
        const float low  = ws.lpFilter.process (mono);
        const float high = ws.hpFilter.process (mono);

        // Write high band into decorrelation delay
        ws.decoBuffer[ws.decoWritePos] = high;
        const int readPos = (ws.decoWritePos - decoD + kDecoDelayMax) & (kDecoDelayMax - 1);
        const float highDeco = ws.decoBuffer[readPos];
        ws.decoWritePos = (ws.decoWritePos + 1) & (kDecoDelayMax - 1);

        // Mid/side on high band
        const float mid  = (high + highDeco) * 0.5f;
        const float side = (high - highDeco) * 0.5f;

        // Width scaling: 0→0 (mono), 0.5→1 (natural), 1.0→2 (wide)
        const float sideGain = width * 2.0f;

        outL = low + mid + side * sideGain;
        outR = low + mid - side * sideGain;
    };

    // ── Per-sample synthesis ───────────────────────────────────────────────

    auto renderSamples = [&] (int start, int end)
    {
        for (int s = start; s < end; ++s)
        {
            const float gainThisSample = gainSmooth.getNextValue();

            const bool bodyActive      = isPlaying;
            const bool declickActive   = (bodyFadeLeft > 0);
            const bool samplesPlaying  = transientPlaying || resonantPlaying || noiseSamplePlaying;

            if (!bodyActive && !declickActive && !samplesPlaying)
                continue;

            float mixL = 0.0f, mixR = 0.0f;

            // ── Transient sample layer ─────────────────────────────────────
            if (transientPlaying && layerTransient && transientData != nullptr)
            {
                const float tNorm = (float) transientPlayPos / (float) transientLen;
                const float ampEnv = (float) transientAmpEnvelope.evaluate ((double) tNorm);
                const float mono = transientData[transientPlayPos] * ampEnv * transientLevel;

                float wL, wR;
                applyWidth (mono, transientWidth, widthStates[0], wL, wR);
                mixL += wL;
                mixR += wR;
            }
            if (transientPlaying)
            {
                ++transientPlayPos;
                if (transientPlayPos >= transientLen)
                    transientPlaying = false;
            }

            // ── Body oscillator layer ──────────────────────────────────────
            float bodyOut = 0.0f;

            if (bodyActive)
            {
                const double t = envTime;
                envTime += 1.0;

                const double pitchEnv = pitchEnvelope.getNextSample();
                const double currentFreq = bodyFreq * (1.0 + (freqRatio - 1.0) * pitchEnv);

                phase += currentFreq / sr;
                if (phase >= 1.0) phase -= 1.0;

                // Amp envelope replaces hardcoded exponential decay
                const double envNorm = t / bodyEnvDurSamples;
                const float bodyAmp = (float) bodyAmpEnvelope.evaluate (
                    std::min (envNorm, 1.0));

                const float rawBody = (float) std::sin (
                    juce::MathConstants<double>::twoPi * phase) * bodyAmp;

                if (bodyOnsetGain < 1.0)
                    bodyOnsetGain = std::min (1.0, bodyOnsetGain + bodyOnsetRate);

                bodyOut        = rawBody * (float) bodyOnsetGain;
                bodyLastOutput = bodyOut;

                // Stop when amp envelope is done and past minimum duration
                if (t >= voiceStopTime && bodyAmp < 1e-5f)
                    isPlaying = false;
            }

            if (layerBody)
            {
                const float mono = bodyOut * bodyLevel;
                float wL, wR;
                applyWidth (mono, bodyWidth, widthStates[1], wL, wR);
                mixL += wL;
                mixR += wR;
            }

            // Body declick tail (mono, no width processing needed)
            if (declickActive)
            {
                const float fadeGain = (float) bodyFadeLeft / (float) kDeclickSamples;
                const float fade = bodyFadeSample * fadeGain;
                mixL += fade;
                mixR += fade;
                --bodyFadeLeft;
            }

            // ── Resonant sample layer ──────────────────────────────────────
            if (resonantPlaying && layerResonant && resonantData != nullptr)
            {
                const float tNorm = (float) resonantPlayPos / (float) resonantLen;
                const float ampEnv = (float) resonantAmpEnvelope.evaluate ((double) tNorm);
                const float mono = resonantData[resonantPlayPos] * ampEnv * resonantLevel;

                float wL, wR;
                applyWidth (mono, resonantWidth, widthStates[2], wL, wR);
                mixL += wL;
                mixR += wR;
            }
            if (resonantPlaying)
            {
                ++resonantPlayPos;
                if (resonantPlayPos >= resonantLen)
                    resonantPlaying = false;
            }

            // ── Noise layer (generated) ────────────────────────────────────
            if (bodyActive && layerNoise)
            {
                const double envNorm = (envTime - 1.0) / noiseEnvDurSamples;
                const float noiseAmp = (float) noiseAmpEnvelope.evaluate (
                    std::min (envNorm, 1.0));

                if (noiseAmp > 1e-6f)
                {
                    const float rawNoise = (random.nextFloat() * 2.0f - 1.0f)
                                           * noiseLevel * noiseAmp;
                    const float mono = noiseBrightFilter.process (
                        noiseTypeFilter.process (rawNoise));

                    float wL, wR;
                    applyWidth (mono, noiseWidth, widthStates[3], wL, wR);
                    mixL += wL;
                    mixR += wR;
                }
            }

            // ── Noise sample layer (loaded sample, uses noise amp envelope) ─
            if (noiseSamplePlaying && layerNoise && noiseSmpData != nullptr)
            {
                const float tNorm = (float) noiseSamplePlayPos / (float) noiseSmpLen;
                const float ampEnv = (float) noiseAmpEnvelope.evaluate ((double) tNorm);
                const float mono = noiseSmpData[noiseSamplePlayPos] * ampEnv * noiseLevel;

                float wL, wR;
                applyWidth (mono, noiseWidth, widthStates[3], wL, wR);
                mixL += wL;
                mixR += wR;
            }
            if (noiseSamplePlaying)
            {
                ++noiseSamplePlayPos;
                if (noiseSamplePlayPos >= noiseSmpLen)
                    noiseSamplePlaying = false;
            }

            // ── Apply smoothed output gain to stereo mix ───────────────────
            if (numCh >= 2)
            {
                buffer.addSample (0, s, mixL * gainThisSample);
                buffer.addSample (1, s, mixR * gainThisSample);
            }
            else
            {
                // Mono bus: sum to mono
                buffer.addSample (0, s, (mixL + mixR) * 0.5f * gainThisSample);
            }
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

    // Update playback position for UI (once per block, after rendering)
    if (isPlaying || transientPlaying || resonantPlaying || noiseSamplePlaying)
        playbackPositionSec.store ((float) (envTime / sr), std::memory_order_relaxed);
    else
        playbackPositionSec.store (-1.0f, std::memory_order_relaxed);
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
    // Voice duration is based on pitchDecay (envelope time).
    // Generous multiplier so DAW doesn't cut the bounce early.
    const double pitchDecaySec = (double) pPitchDecay->load() * 0.001;
    return std::max (2.0, pitchDecaySec * 7.0);
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
            saveEnvelope ("PitchEnvelope",         pitchEnvelope);
            saveEnvelope ("TransientAmpEnvelope", transientAmpEnvelope);
            saveEnvelope ("BodyAmpEnvelope",      bodyAmpEnvelope);
            saveEnvelope ("NoiseAmpEnvelope",     noiseAmpEnvelope);
            saveEnvelope ("RoomAmpEnvelope",      roomAmpEnvelope);
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
        loadEnvelope ("PitchEnvelope",         pitchEnvelope);
        loadEnvelope ("TransientAmpEnvelope", transientAmpEnvelope);
        loadEnvelope ("BodyAmpEnvelope",      bodyAmpEnvelope);
        loadEnvelope ("NoiseAmpEnvelope",     noiseAmpEnvelope);
        loadEnvelope ("RoomAmpEnvelope",      roomAmpEnvelope);
    }
}

// =============================================================================
// Plugin entry point
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SnareMakerAudioProcessor();
}
