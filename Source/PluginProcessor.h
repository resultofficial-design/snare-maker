#pragma once
#include <JuceHeader.h>

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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Raw parameter pointers – safe to read from the audio thread
    std::atomic<float>* pBodyFreq    { nullptr };
    std::atomic<float>* pPitchAmount { nullptr };
    std::atomic<float>* pPitchDecay   { nullptr };
    std::atomic<float>* pPitchCurve   { nullptr };   // Phase 2a: 0=Exp, 1=Lin, 2=Log
    std::atomic<float>* pPhaseOffset  { nullptr };   // Phase 2a: 0–360°
    std::atomic<float>* pNoiseLevel   { nullptr };
    std::atomic<float>* pNoiseDecay  { nullptr };
    std::atomic<float>* pOutputGain  { nullptr };

    // Synthesis state (audio thread only)
    bool   isPlaying         { false };
    double envTime           { 0.0 };   // elapsed time in samples since trigger
    double phase             { 0.0 };   // normalised oscillator phase [0, 1)
    double currentSampleRate { 44100.0 };

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnareMakerAudioProcessor)
};
