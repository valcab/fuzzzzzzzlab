#include "PluginEditor.h"

FuzzVSTAudioProcessorEditor::FuzzVSTAudioProcessorEditor(FuzzVSTAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(720, 320);

    titleLabel.setText("FuzzVST", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    setupSlider(inputSlider, " dB");
    setupSlider(driveSlider, "");
    setupSlider(toneSlider, "");
    setupSlider(biasSlider, "");
    setupSlider(octaveSlider, "");
    setupSlider(mixSlider, "");
    setupSlider(outputSlider, " dB");

    fuzzTypeBox.addItemList({ "Hard Clip", "Octave", "Gated", "Muff", "Wavefold", "Bitcrush" }, 1);
    oversamplingBox.addItemList({ "1x", "2x", "4x" }, 1);

    addAndMakeVisible(fuzzTypeBox);
    addAndMakeVisible(oversamplingBox);
    addAndMakeVisible(gateButton);
    addAndMakeVisible(cabButton);

    fuzzTypeAttachment = std::make_unique<ComboAttachment>(audioProcessor.apvts, "fuzzType", fuzzTypeBox);
    oversamplingAttachment = std::make_unique<ComboAttachment>(audioProcessor.apvts, "oversampling", oversamplingBox);

    inputAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "input", inputSlider);
    driveAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "drive", driveSlider);
    toneAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "tone", toneSlider);
    biasAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "bias", biasSlider);
    octaveAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "octaveBlend", octaveSlider);
    mixAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "output", outputSlider);

    gateAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "gate", gateButton);
    cabAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "cab", cabButton);
}

void FuzzVSTAudioProcessorEditor::setupSlider(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setTextValueSuffix(suffix);
    slider.setPopupDisplayEnabled(true, false, this);
    addAndMakeVisible(slider);
}

void FuzzVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff131313));

    auto area = getLocalBounds().reduced(10);
    g.setColour(juce::Colour(0xff202020));
    g.fillRoundedRectangle(area.toFloat(), 12.0f);

    g.setColour(juce::Colour(0xffff8c42));
    g.drawRoundedRectangle(area.toFloat(), 12.0f, 2.0f);
}

void FuzzVSTAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);

    titleLabel.setBounds(area.removeFromTop(36));
    area.removeFromTop(8);

    auto topRow = area.removeFromTop(38);
    fuzzTypeBox.setBounds(topRow.removeFromLeft(220));
    topRow.removeFromLeft(12);
    oversamplingBox.setBounds(topRow.removeFromLeft(90));
    topRow.removeFromLeft(20);
    gateButton.setBounds(topRow.removeFromLeft(80));
    topRow.removeFromLeft(6);
    cabButton.setBounds(topRow.removeFromLeft(80));

    area.removeFromTop(16);

    std::array<juce::Slider*, 7> sliders {
        &inputSlider,
        &driveSlider,
        &toneSlider,
        &biasSlider,
        &octaveSlider,
        &mixSlider,
        &outputSlider
    };

    const int sliderWidth = area.getWidth() / static_cast<int>(sliders.size());

    for (auto* slider : sliders)
        slider->setBounds(area.removeFromLeft(sliderWidth).reduced(6));
}
