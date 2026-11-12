#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

class WahAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WahAudioProcessorEditor (WahAudioProcessor&);
    ~WahAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Sichtfenster: Wah-Pedal von der Seite, die Wippe folgt der Pedalstellung
    struct PedalWindow : public juce::Component
    {
        float pos = 0.5f;
        bool on = true;
        void paint (juce::Graphics&) override;
    };

    struct Knob { juce::Slider slider; juce::String name; bool big = false; std::unique_ptr<SliderAttachment> attachment; };
    Knob& addKnob (const juce::String& paramID, const juce::String& name, bool big);
    void setupSwitch (juce::ToggleButton&, const juce::String& paramID, const juce::String& onText,
                      const juce::String& offText, juce::Colour lampColour);
    void timerCallback() override;
    void renderTextures();
    void drawEngraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Justification);

    WahAudioProcessor& proc;
    VintageLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob* pedalKnob = nullptr; Knob* resKnob = nullptr;
    Knob* modeKnob = nullptr;  Knob* typeKnob = nullptr; Knob* intKnob = nullptr; Knob* tempoKnob = nullptr; Knob* outKnob = nullptr;
    juce::ToggleButton dirSwitch, onSwitch;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetBox;
    PedalWindow pedal;
    juce::Label freqLabel;

    juce::Rectangle<int> faceplate, presetArea, lampBezel, pilotArea, divider;
    int lastMode = -1;
    std::array<juce::Rectangle<int>, 2> switchLabels;
    juce::Image tolexImage, faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WahAudioProcessorEditor)
};
