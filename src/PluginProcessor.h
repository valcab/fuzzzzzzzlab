#pragma once

#include <JuceHeader.h>
#include "dsp/FuzzAlgorithms.h"

class FuzzVSTAudioProcessor final : public juce::AudioProcessor
{
public:
    FuzzVSTAudioProcessor();
    ~FuzzVSTAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    enum FuzzType
    {
        hardClip = 0,
        octave,
        gated,
        muff,
        wavefold,
        bitcrush
    };

    using Coeff = juce::dsp::IIR::Coefficients<float>;
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, Coeff>;

    void updateParameters();
    void configureOversampling(int factor);
    void processNonLinearBlock(juce::dsp::AudioBlock<float>& block);

    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> outputGain;
    juce::dsp::DryWetMixer<float> dryWet;

    Filter dcBlocker;
    juce::dsp::StateVariableTPTFilter<float> toneFilter;
    juce::dsp::StateVariableTPTFilter<float> cabFilter;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling2x;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4x;

    fuzz::HardClipFuzz hardClipFuzz;
    fuzz::OctaveFuzz octaveFuzz;
    fuzz::GatedFuzz gatedFuzz;
    fuzz::MuffFuzz muffFuzz;
    fuzz::WavefolderFuzz wavefoldFuzz;
    fuzz::BitcrushFuzz bitcrushFuzz;

    fuzz::FuzzParameters fuzzParams;

    fuzz::FuzzAlgorithm* activeAlgorithm = &hardClipFuzz;
    int activeOversamplingFactor = 2;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FuzzVSTAudioProcessor)
};
