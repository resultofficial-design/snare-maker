#pragma once
#include <JuceHeader.h>
#include "BiquadFilter.h"      // Phase 2b: noise filter (header-only, no heap alloc)
#include "FlexibleEnvelope.h"  // Phase 7-1: dynamic N-point pitch envelope

class SnareMakerAudioProcessor : public juce::AudioProcessor
{
public:
    SnareMakerAudioProcessor();
    ~SnareMakerAudioProcessor() override;

    // ── AudioProcessor interface ───────────────────────────────────────────
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool   acceptsMidi()  const override;
    bool   producesMidi() const override;
    bool   isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int  getNumPrograms() override;
    int  getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ── Public state ───────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;

    // Dynamic N-point envelopes (shared with EnvelopeEditor for direct access).
    // Points are edited by the UI; audio thread reads under envelopeLock.
    FlexibleEnvelope pitchEnvelope;
    FlexibleEnvelope transientAmpEnvelope;
    FlexibleEnvelope bodyAmpEnvelope;
    FlexibleEnvelope noiseAmpEnvelope;
    FlexibleEnvelope resonantAmpEnvelope;
    FlexibleEnvelope roomAmpEnvelope;
    juce::SpinLock   envelopeLock;

    // Global waveform display mode (0 = Simple, 1 = True).
    // Shared across all EnvelopeEditor instances.
    std::atomic<int> waveformDisplayMode { 1 };   // default = True

    // ── Layer enabled flags (written by editor, read by audio thread) ───────
    std::atomic<bool> transientEnabled { true };
    std::atomic<bool> bodyEnabled      { true };
    std::atomic<bool> resonantEnabled  { true };
    std::atomic<bool> noiseEnabled     { true };

    // ── Playback position (written by audio thread, read by UI) ─────────────
    // Elapsed time in seconds since last note-on.  Negative = not playing.
    std::atomic<float> playbackPositionSec { -1.0f };

    // ── Sample loading (Phase 8) ─────────────────────────────────────────────
    juce::AudioFormatManager formatManager;

    juce::AudioBuffer<float> transientSampleBuffer;
    juce::AudioBuffer<float> resonantSampleBuffer;
    juce::AudioBuffer<float> noiseSampleBuffer;
    juce::AudioBuffer<float> roomIRBuffer;

    juce::String transientSamplePath;
    juce::String resonantSamplePath;
    juce::String noiseSamplePath;
    juce::String roomIRPath;

    bool loadSampleFromFile (const juce::String& filePath,
                             juce::AudioBuffer<float>& dest,
                             juce::String& destPath);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── Raw parameter pointers (safe to read from audio thread) ───────────

    // Body oscillator (Phase 1 / 2a)
    std::atomic<float>* pBodyFreq      { nullptr };
    std::atomic<float>* pPitchAmount   { nullptr };
    std::atomic<float>* pPitchDecay    { nullptr };
    std::atomic<float>* pPhaseOffset   { nullptr };

    // Noise level (Phase 1)
    std::atomic<float>* pNoiseLevel    { nullptr };

    // Per-layer volume knobs (Phase 9)
    std::atomic<float>* pTransientLevel { nullptr };
    std::atomic<float>* pBodyLevel      { nullptr };
    std::atomic<float>* pResonantLevel  { nullptr };
    std::atomic<float>* pRoomLevel      { nullptr };

    // Per-layer width (Phase 10)
    std::atomic<float>* pTransientWidth { nullptr };
    std::atomic<float>* pBodyWidth      { nullptr };
    std::atomic<float>* pResonantWidth  { nullptr };
    std::atomic<float>* pNoiseWidth     { nullptr };
    std::atomic<float>* pRoomWidth      { nullptr };

    // Noise ADSR (Phase 2b)
    std::atomic<float>* pNoiseAttack   { nullptr };
    std::atomic<float>* pNoiseDecay    { nullptr };
    std::atomic<float>* pNoiseSustain  { nullptr };
    std::atomic<float>* pNoiseRelease  { nullptr };

    // Noise filter (Phase 2b)
    std::atomic<float>* pNoiseFiltType { nullptr };
    std::atomic<float>* pNoiseFiltFreq { nullptr };
    std::atomic<float>* pNoiseFiltQ    { nullptr };

    // Noise brightness (Phase 2b)
    std::atomic<float>* pNoiseBright   { nullptr };

    // Output
    std::atomic<float>* pOutputGain    { nullptr };

    // ── Output gain smoother (Phase 2c) ───────────────────────────────────
    // Prevents zipper noise when the gain knob is moved or automated.
    // Advanced once per sample so it stays in sync with the buffer position
    // even during silent blocks.
    juce::SmoothedValue<float> gainSmooth;

    // ── Body oscillator state (audio thread only) ──────────────────────────
    bool   isPlaying         { false };
    double envTime           { 0.0 };   // elapsed samples since trigger
    double phase             { 0.0 };   // normalised oscillator phase [0, 1)
    double currentSampleRate { 44100.0 };

    // ── Body retrigger declick (Phase 2c) ─────────────────────────────────
    //
    // Problem: when a note-on arrives while the body is still decaying, the
    // previous output sample is abruptly cut (e.g. −14 dBFS jump) producing
    // an audible click.
    //
    // Solution – two-sided crossfade over kDeclickSamples (~3 ms):
    //   OLD body: frozen last-output sample fades linearly 1→0 (bodyFadeSample)
    //   NEW body: onset gain ramps linearly 0→1 (bodyOnsetGain)
    // The combined output has no discontinuity at the retrigger boundary.
    //
    static constexpr int kDeclickSamples = 128; // ~2.9 ms at 44.1 kHz

    float  bodyLastOutput  { 0.0f }; // tracks last rendered body sample
    float  bodyFadeSample  { 0.0f }; // frozen level to fade out on retrigger
    int    bodyFadeLeft    { 0 };    // countdown: old-body fade samples remaining
    double bodyOnsetGain   { 1.0 };  // [0,1] onset multiplier for new body
    double bodyOnsetRate   { 0.0 };  // per-sample increment for bodyOnsetGain

    // ── Sample playback state (audio thread only) ─────────────────────────
    int  transientPlayPos     { 0 };
    int  resonantPlayPos      { 0 };
    int  noiseSamplePlayPos   { 0 };
    bool transientPlaying     { false };
    bool resonantPlaying      { false };
    bool noiseSamplePlaying   { false };

    // ── Noise filter instances (Phase 2b, audio thread only) ──────────────
    BiquadFilter noiseTypeFilter;     // HP / BP / LP
    BiquadFilter noiseBrightFilter;   // high-shelf tilt EQ

    juce::Random random;

    // ── Per-layer stereo width processing (Phase 10) ────────────────────
    // Crossover at ~200 Hz splits each layer into low (mono) + high (width-processed).
    // Small decorrelation delay on the high band creates stereo content from mono sources.
    static constexpr int kWidthLayers     = 5;   // transient, body, resonant, noise, room
    static constexpr int kDecoDelayMax    = 64;  // max decorrelation delay samples
    static constexpr float kCrossoverFreq = 200.0f;

    struct WidthState
    {
        BiquadFilter lpFilter;
        BiquadFilter hpFilter;
        float decoBuffer[kDecoDelayMax] {};
        int   decoWritePos = 0;

        void reset()
        {
            lpFilter.reset();
            hpFilter.reset();
            std::memset (decoBuffer, 0, sizeof (decoBuffer));
            decoWritePos = 0;
        }
    };

    WidthState widthStates[kWidthLayers];
    int decoDelaySamples = 13;   // set in prepareToPlay (~0.3ms)

    // ── Private helpers (Phase 2c modular refactoring) ────────────────────
    void resetVoiceState ();
    void triggerNote     (float phaseOffsetDeg);
    void releaseNote     ();
    void killAll         ();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessor)
};
