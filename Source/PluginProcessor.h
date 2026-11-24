#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>

class DeReverbenjaminM3Processor : public juce::AudioProcessor
{
public:
    DeReverbenjaminM3Processor();
    ~DeReverbenjaminM3Processor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
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

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> currentGainReduction { 1.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Motor Ultra-HD Optimizado: 4096-pt FFT con 75% de solapamiento (4x overlap)
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;     // 4096
    static constexpr int hopSize = fftSize / 4;       // 1024
    static constexpr int numBins = fftSize / 2 + 1;   // 2049

    struct FastOversampledSaturator
    {
        float prevIn = 0.0f;

        static inline float fastTanh(float x)
        {
            if (x < -3.0f) return -1.0f;
            if (x >  3.0f) return  1.0f;
            const float x2 = x * x;
            return x * (27.0f + x2) / (27.0f + 9.0f * x2);
        }

        inline float process2x(float x, float drive)
        {
            const float mid = 0.5f * (prevIn + x);
            prevIn = x;
            return 0.5f * (fastTanh(mid * drive) + fastTanh(x * drive));
        }

        void reset() { prevIn = 0.0f; }
    };

    struct ChannelState
    {
        std::array<float, fftSize> inputBuffer{};
        std::array<float, fftSize> outputAccum{};
        std::array<float, hopSize> outputReady{};
        std::array<float, fftSize * 2> fftData{};

        std::array<float, numBins> prevPower{};
        std::array<float, numBins> reverbEst{};
        std::array<float, numBins> rawGain{};
        std::array<float, numBins> smoothGain{};

        std::array<float, fftSize> dryDelay{};
        int dryWritePos = 0;

        float lowLp = 0.0f;
        float midLoLp = 0.0f;
        float midHiLp = 0.0f;
        float airLp = 0.0f;
        float dryLowLp = 0.0f;

        uint32_t rngState = 2463534242u;
        float noiseHpState = 0.0f;
        float noiseLpState = 0.0f;

        float dcPrevIn = 0.0f;
        float dcPrevOut = 0.0f;

        FastOversampledSaturator satLow, satMid, satHigh, satFull;

        int hopIndex = 0;
        float lastAvgGain = 1.0f;

        inline float nextWhiteNoise()
        {
            uint32_t x = rngState;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            rngState = x;
            return (static_cast<float>(x & 0x00FFFFFFu) / 8388608.0f) - 1.0f;
        }

        void reset(uint32_t seed)
        {
            inputBuffer.fill(0.0f);
            outputAccum.fill(0.0f);
            outputReady.fill(0.0f);
            fftData.fill(0.0f);
            prevPower.fill(0.0f);
            reverbEst.fill(0.0f);
            rawGain.fill(1.0f);
            smoothGain.fill(1.0f);
            dryDelay.fill(0.0f);
            dryWritePos = 0;
            lowLp = midLoLp = midHiLp = airLp = dryLowLp = 0.0f;
            rngState = seed;
            noiseHpState = noiseLpState = 0.0f;
            dcPrevIn = dcPrevOut = 0.0f;
            satLow.reset();
            satMid.reset();
            satHigh.reset();
            satFull.reset();
            hopIndex = 0;
            lastAvgGain = 1.0f;
        }
    };

    void processFrame(ChannelState& state, float amount, float roomDecay, float roomFocus, float smoothness, int mode);

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::vector<ChannelState> channels;

    float fastEnv = 0.0f;
    float slowEnv = 0.0f;
    float tailSmoothGain = 1.0f;

    float dryTransFast = 0.0f;
    float dryTransSlow = 0.0f;
    float transSmooth = 0.0f;

    float lowTransFast = 0.0f;
    float lowTransSlow = 0.0f;
    float subEnv = 0.0f;
    float subPhase = 0.0f;
    float vocalOptoEnv = 0.0f;
    float airGateEnv = 0.0f;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeReverbenjaminM3Processor)
};
