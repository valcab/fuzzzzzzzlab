#include "PluginProcessor.h"
#include "PluginEditor.h"

FuzzVSTAudioProcessor::FuzzVSTAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

void FuzzVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    inputGain.prepare(spec);
    outputGain.prepare(spec);
    dryWet.prepare(spec);

    toneFilter.prepare(spec);
    toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setCutoffFrequency(2000.0f);

    dcBlocker.prepare(spec);
    *dcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);

    cabFilter.prepare(spec);
    cabFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    cabFilter.setCutoffFrequency(5000.0f);

    oversampling2x = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling4x = std::make_unique<juce::dsp::Oversampling<float>>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling2x->initProcessing(static_cast<size_t>(samplesPerBlock));
    oversampling4x->initProcessing(static_cast<size_t>(samplesPerBlock));

    hardClipFuzz.prepare(sampleRate, 2);
    octaveFuzz.prepare(sampleRate, 2);
    gatedFuzz.prepare(sampleRate, 2);
    muffFuzz.prepare(sampleRate, 2);
    wavefoldFuzz.prepare(sampleRate, 2);
    bitcrushFuzz.prepare(sampleRate, 2);

    updateParameters();
}

void FuzzVSTAudioProcessor::releaseResources()
{
}

bool FuzzVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getChannelSet(true, 0);
    const auto output = layouts.getChannelSet(false, 0);

    return input == juce::AudioChannelSet::stereo() && output == juce::AudioChannelSet::stereo();
}

void FuzzVSTAudioProcessor::updateParameters()
{
    const auto drive = apvts.getRawParameterValue("drive")->load();
    const auto tone = apvts.getRawParameterValue("tone")->load();
    const auto bias = apvts.getRawParameterValue("bias")->load();
    const auto outputDb = apvts.getRawParameterValue("output")->load();
    const auto mix = apvts.getRawParameterValue("mix")->load();
    const auto inputDb = apvts.getRawParameterValue("input")->load();
    const auto octaveBlend = apvts.getRawParameterValue("octaveBlend")->load();
    const auto fuzzType = static_cast<int>(apvts.getRawParameterValue("fuzzType")->load());
    const auto oversampling = static_cast<int>(apvts.getRawParameterValue("oversampling")->load());
    const auto gate = apvts.getRawParameterValue("gate")->load() > 0.5f;
    const auto cabOn = apvts.getRawParameterValue("cab")->load() > 0.5f;

    fuzzParams.drive = drive;
    fuzzParams.tone = tone;
    fuzzParams.bias = bias;
    fuzzParams.octaveBlend = octaveBlend;
    fuzzParams.gateEnabled = gate;

    inputGain.setGainDecibels(inputDb);
    outputGain.setGainDecibels(outputDb);
    dryWet.setWetMixProportion(mix);

    const auto toneHz = juce::jmap(tone, 0.0f, 1.0f, 500.0f, 8000.0f);
    toneFilter.setCutoffFrequency(toneHz);

    if (fuzzType == muff)
        toneFilter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    else
        toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    cabFilter.setCutoffFrequency(cabOn ? 5000.0f : 22000.0f);

    switch (fuzzType)
    {
        case hardClip: activeAlgorithm = &hardClipFuzz; break;
        case octave: activeAlgorithm = &octaveFuzz; break;
        case gated: activeAlgorithm = &gatedFuzz; break;
        case muff: activeAlgorithm = &muffFuzz; break;
        case wavefold: activeAlgorithm = &wavefoldFuzz; break;
        case bitcrush: activeAlgorithm = &bitcrushFuzz; break;
        default: activeAlgorithm = &hardClipFuzz; break;
    }

    switch (oversampling)
    {
        case 0: activeOversamplingFactor = 1; break;
        case 1: activeOversamplingFactor = 2; break;
        case 2: activeOversamplingFactor = 4; break;
        default: activeOversamplingFactor = 2; break;
    }
}

void FuzzVSTAudioProcessor::processNonLinearBlock(juce::dsp::AudioBlock<float>& block)
{
    for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
    {
        auto* data = block.getChannelPointer(ch);

        for (size_t i = 0; i < block.getNumSamples(); ++i)
            data[i] = activeAlgorithm->processSample(data[i], static_cast<int>(ch), fuzzParams);
    }
}

void FuzzVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    updateParameters();

    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalInputChannels; i < totalOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    dryWet.pushDrySamples(block);

    inputGain.process(context);
    dcBlocker.process(context);

    if (activeOversamplingFactor == 4 && oversampling4x != nullptr)
    {
        auto upsampled = oversampling4x->processSamplesUp(block);
        processNonLinearBlock(upsampled);
        oversampling4x->processSamplesDown(block);
    }
    else if (activeOversamplingFactor == 2 && oversampling2x != nullptr)
    {
        auto upsampled = oversampling2x->processSamplesUp(block);
        processNonLinearBlock(upsampled);
        oversampling2x->processSamplesDown(block);
    }
    else
    {
        processNonLinearBlock(block);
    }

    toneFilter.process(context);
    cabFilter.process(context);

    outputGain.process(context);
    dryWet.mixWetSamples(block);
}

juce::AudioProcessorValueTreeState::ParameterLayout FuzzVSTAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("input", "Input", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Drive", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tone", "Tone", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("bias", "Bias", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("octaveBlend", "Octave Blend", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("output", "Output", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), -3.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "fuzzType",
        "Fuzz Type",
        juce::StringArray { "Hard Clip", "Octave", "Gated", "Muff", "Wavefold", "Bitcrush" },
        0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "oversampling",
        "Oversampling",
        juce::StringArray { "1x", "2x", "4x" },
        1));

    params.push_back(std::make_unique<juce::AudioParameterBool>("gate", "Noise Gate", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>("cab", "Cab Sim", true));

    return { params.begin(), params.end() };
}

void FuzzVSTAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FuzzVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor* FuzzVSTAudioProcessor::createEditor()
{
    return new FuzzVSTAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FuzzVSTAudioProcessor();
}
