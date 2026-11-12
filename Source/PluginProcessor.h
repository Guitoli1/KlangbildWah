#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "WahDsp.h"

class WahAudioProcessor : public juce::AudioProcessor
{
public:
    WahAudioProcessor();
    ~WahAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Pegelausgleich je Typ (gemessen): Wah-Ausgang ≈ Eingang bei halber Pedalstellung
    static constexpr std::array<float, 4> makeup { 2.79f, 3.77f, 1.44f, 2.40f };

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> position { 0.5f }, frequency { 900.0f }, vuOut { 0 }, peakOut { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<wah::SVF, 2> filters;
    wah::Envelope env;
    double lfoPhase = 0, posState = 0.5, posCoef = 0.01;
    juce::SmoothedValue<float> smOut, smOn, smRes;
    float vuState = 0, vuCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WahAudioProcessor)
};
