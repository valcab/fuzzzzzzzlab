#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace fuzz
{
struct FuzzParameters
{
    float drive = 0.6f;
    float tone = 0.5f;
    float bias = 0.0f;
    float octaveBlend = 0.0f;
    bool gateEnabled = false;
};

class FuzzAlgorithm
{
public:
    virtual ~FuzzAlgorithm() = default;

    virtual void prepare(double sampleRateToUse, int channels)
    {
        sampleRate = sampleRateToUse;
        numChannels = juce::jlimit(1, 2, channels);
        reset();
    }

    virtual void reset() {}

    virtual float processSample(float x, int channel, const FuzzParameters& params) = 0;

protected:
    double sampleRate = 44100.0;
    int numChannels = 2;
};

class HardClipFuzz final : public FuzzAlgorithm
{
public:
    // Inspired by transistor fuzz clipping stages, with aggressive hard limiting.
    float processSample(float x, int /*channel*/, const FuzzParameters& params) override
    {
        const float gain = 1.0f + params.drive * 35.0f;
        const float threshold = juce::jmap(params.drive, 0.0f, 1.0f, 0.85f, 0.22f);
        return juce::jlimit(-threshold, threshold, x * gain) / juce::jmax(threshold, 1.0e-4f);
    }
};

class OctaveFuzz final : public FuzzAlgorithm
{
public:
    // Inspired by octave-up fuzz units that rectify the waveform before clipping.
    float processSample(float x, int /*channel*/, const FuzzParameters& params) override
    {
        const float gain = 1.0f + params.drive * 28.0f;
        const float base = std::tanh(x * gain);
        const float rectified = std::abs(x) * (1.0f + params.drive * 20.0f);
        const float octave = std::tanh(rectified);
        return juce::jlimit(-1.2f, 1.2f, juce::jmap(params.octaveBlend, base, octave));
    }
};

class GatedFuzz final : public FuzzAlgorithm
{
public:
    void reset() override
    {
        env.fill(0.0f);
    }

    // Inspired by starved-bias fuzz circuits with asymmetric clipping and gate-like decay.
    float processSample(float x, int channel, const FuzzParameters& params) override
    {
        const auto ch = static_cast<size_t>(juce::jlimit(0, 1, channel));
        const float gain = 1.0f + params.drive * 45.0f;
        const float biased = x * gain + params.bias * 0.8f;

        const float pos = std::tanh(biased * 1.4f);
        const float neg = std::tanh(biased * 0.65f);
        float y = (biased >= 0.0f ? pos : neg) - params.bias * 0.15f;

        const float detector = std::abs(y);
        env[ch] = detector > env[ch] ? detector : (0.9985f * env[ch] + 0.0015f * detector);

        if (params.gateEnabled)
        {
            const float threshold = juce::jmap(params.bias, -1.0f, 1.0f, 0.03f, 0.18f);
            if (env[ch] < threshold)
                y = 0.0f;
        }

        return juce::jlimit(-1.2f, 1.2f, y);
    }

private:
    std::array<float, 2> env { 0.0f, 0.0f };
};

class MuffFuzz final : public FuzzAlgorithm
{
public:
    void reset() override
    {
        lowState.fill(0.0f);
        highState.fill(0.0f);
    }

    // Inspired by multi-stage soft clipping with a passive-style tone blend.
    float processSample(float x, int channel, const FuzzParameters& params) override
    {
        const auto ch = static_cast<size_t>(juce::jlimit(0, 1, channel));
        const float gain = 1.0f + params.drive * 30.0f;
        const float clipped = std::atan(x * gain) * (2.0f / juce::MathConstants<float>::pi);

        const float lpCut = juce::jmap(params.tone, 0.0f, 1.0f, 700.0f, 5000.0f);
        const float hpCut = juce::jmap(params.tone, 0.0f, 1.0f, 150.0f, 1400.0f);
        const float lpAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * lpCut / static_cast<float>(sampleRate));
        const float hpAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * hpCut / static_cast<float>(sampleRate));

        lowState[ch] = (1.0f - lpAlpha) * clipped + lpAlpha * lowState[ch];
        highState[ch] = hpAlpha * (highState[ch] + clipped - prevIn[ch]);
        prevIn[ch] = clipped;

        const float toneBlend = juce::jmap(params.tone, 0.0f, 1.0f, 0.75f, 0.25f);
        return juce::jlimit(-1.1f, 1.1f, lowState[ch] * toneBlend + highState[ch] * (1.0f - toneBlend));
    }

private:
    std::array<float, 2> lowState { 0.0f, 0.0f };
    std::array<float, 2> highState { 0.0f, 0.0f };
    std::array<float, 2> prevIn { 0.0f, 0.0f };
};

class WavefolderFuzz final : public FuzzAlgorithm
{
public:
    // Inspired by folded transfer curves from modular wavefolder circuits.
    float processSample(float x, int /*channel*/, const FuzzParameters& params) override
    {
        float y = x * (1.0f + params.drive * 50.0f);

        for (int i = 0; i < 6; ++i)
        {
            if (y > 1.0f)
                y = 2.0f - y;
            else if (y < -1.0f)
                y = -2.0f - y;
            else
                break;
        }

        return y;
    }
};

class BitcrushFuzz final : public FuzzAlgorithm
{
public:
    void reset() override
    {
        heldSample.fill(0.0f);
        sampleCounter.fill(0);
    }

    // Digital fuzz via reduced bit depth and sample-rate reduction.
    float processSample(float x, int channel, const FuzzParameters& params) override
    {
        const auto ch = static_cast<size_t>(juce::jlimit(0, 1, channel));
        const float driven = std::tanh(x * (1.0f + params.drive * 25.0f));

        const int bitDepth = static_cast<int>(juce::jmap(params.tone, 0.0f, 1.0f, 4.0f, 12.0f));
        const int holdSamples = static_cast<int>(juce::jmap(params.drive, 0.0f, 1.0f, 1.0f, 24.0f));

        if (sampleCounter[ch]++ >= holdSamples)
        {
            sampleCounter[ch] = 0;
            const float scale = static_cast<float>((1 << bitDepth) - 1);
            heldSample[ch] = std::round(((driven * 0.5f) + 0.5f) * scale) / scale;
            heldSample[ch] = heldSample[ch] * 2.0f - 1.0f;
        }

        return heldSample[ch];
    }

private:
    std::array<float, 2> heldSample { 0.0f, 0.0f };
    std::array<int, 2> sampleCounter { 0, 0 };
};
} // namespace fuzz
