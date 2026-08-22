#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../Common/Constants.h"
#include "../DSP/BiquadFilter.h"
#include <array>
#include <random>

namespace TeethDrummer
{
    struct DrumVoice
    {
        bool active{false};
        DrumPad pad{DrumPad::None};
        float velocity{1.0f};
        float phase{0.0f};
        float phaseInc{0.0f};
        float env{0.0f};
        float envDecay{0.999f};
        float pitchEnv{0.0f};
        float pitchDecay{0.99f};
        float baseFreq{100.0f};
        float startFreq{250.0f};
        
        // Secondary noise layer
        float noiseEnv{0.0f};
        float noiseDecay{0.999f};
        BiquadFilter filter;
    };

    class DrumSampler
    {
    public:
        DrumSampler();

        void prepare(double sampleRate);
        void reset();

        void trigger(DrumPad pad, float velocity);
        void renderBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

        void setEnabled(bool enabled) noexcept { isEnabled = enabled; }
        bool getEnabled() const noexcept { return isEnabled; }

        void setVolume(float gainLinear) noexcept { masterGain = std::clamp(gainLinear, 0.0f, 2.0f); }
        float getVolume() const noexcept { return masterGain; }

    private:
        double currentSampleRate{DSPConfig::DefaultSampleRate};
        bool isEnabled{true};
        float masterGain{0.8f};

        std::array<DrumVoice, static_cast<size_t>(DrumPad::Count)> voices;
        std::minstd_rand noiseGen{1337};
        std::uniform_real_distribution<float> noiseDist{-1.0f, 1.0f};

        inline float getWhiteNoise() noexcept { return noiseDist(noiseGen); }
    };
}

