#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FuzzVSTAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit FuzzVSTAudioProcessorEditor(FuzzVSTAudioProcessor&);
    ~FuzzVSTAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void setupSlider(juce::Slider& slider, const juce::String& suffix);

    FuzzVSTAudioProcessor& audioProcessor;

    juce::Slider inputSlider;
    juce::Slider driveSlider;
    juce::Slider toneSlider;
    juce::Slider biasSlider;
    juce::Slider octaveSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;

    juce::ComboBox fuzzTypeBox;
    juce::ComboBox oversamplingBox;

    juce::ToggleButton gateButton { "Gate" };
    juce::ToggleButton cabButton { "Cab" };

    juce::Label titleLabel;

    std::unique_ptr<SliderAttachment> inputAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> toneAttachment;
    std::unique_ptr<SliderAttachment> biasAttachment;
    std::unique_ptr<SliderAttachment> octaveAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    std::unique_ptr<ComboAttachment> fuzzTypeAttachment;
    std::unique_ptr<ComboAttachment> oversamplingAttachment;

    std::unique_ptr<ButtonAttachment> gateAttachment;
    std::unique_ptr<ButtonAttachment> cabAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FuzzVSTAudioProcessorEditor)
};
