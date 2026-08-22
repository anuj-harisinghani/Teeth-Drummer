#include "DrumSampler.h"
#include <cmath>
#include <numbers>

namespace TeethDrummer
{
    DrumSampler::DrumSampler()
    {
        prepare(DSPConfig::DefaultSampleRate);
    }

    void DrumSampler::prepare(double sampleRate)
    {
        currentSampleRate = (sampleRate > 0.0) ? sampleRate : DSPConfig::DefaultSampleRate;
        const float sr = static_cast<float>(currentSampleRate);

        for (auto& v : voices)
        {
            v.active = false;
            v.filter.configure(BiquadFilter::Type::HighPass, 2000.0f, 0.707f, sr);
        }

        reset();
    }

    void DrumSampler::reset()
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.env = 0.0f;
            v.noiseEnv = 0.0f;
            v.filter.reset();
        }
    }

    void DrumSampler::trigger(DrumPad pad, float velocity)
    {
        if (!isEnabled || pad == DrumPad::None)
            return;

        const size_t idx = static_cast<size_t>(pad);
        if (idx >= voices.size())
            return;

        const float sr = static_cast<float>(currentSampleRate);
        auto& v = voices[idx];
        v.pad = pad;
        v.velocity = std::clamp(velocity, 0.05f, 1.0f);
        v.active = true;
        v.phase = 0.0f;
        v.env = 1.0f;

        switch (pad)
        {
            case DrumPad::Kick:
            {
                v.startFreq = 160.0f;
                v.baseFreq = 48.0f;
                v.pitchEnv = 1.0f;
                // Decay ~250ms
                v.envDecay = std::exp(-1.0f / (0.25f * sr));
                // Pitch drop in ~35ms
                v.pitchDecay = std::exp(-1.0f / (0.035f * sr));
                v.noiseEnv = 0.4f; // Initial transient click
                v.noiseDecay = std::exp(-1.0f / (0.005f * sr));
                v.filter.configure(BiquadFilter::Type::LowPass, 1200.0f, 0.707f, sr);
                break;
            }

            case DrumPad::Snare:
            {
                v.startFreq = 260.0f;
                v.baseFreq = 180.0f;
                v.pitchEnv = 1.0f;
                // Body decay ~120ms
                v.envDecay = std::exp(-1.0f / (0.12f * sr));
                v.pitchDecay = std::exp(-1.0f / (0.02f * sr));
                // Snare wire noise decay ~180ms
                v.noiseEnv = 1.0f;
                v.noiseDecay = std::exp(-1.0f / (0.18f * sr));
                v.filter.configure(BiquadFilter::Type::HighPass, 1500.0f, 0.707f, sr);
                break;
            }

            case DrumPad::ClosedHat:
            {
                v.baseFreq = 8000.0f;
                // Tight decay ~35ms
                v.env = 0.0f;
                v.noiseEnv = 1.0f;
                v.noiseDecay = std::exp(-1.0f / (0.035f * sr));
                v.filter.configure(BiquadFilter::Type::HighPass, 7000.0f, 1.2f, sr);
                
                // Closed hat chokes open hat!
                voices[static_cast<size_t>(DrumPad::OpenHat)].active = false;
                voices[static_cast<size_t>(DrumPad::OpenHat)].noiseEnv = 0.0f;
                break;
            }

            case DrumPad::OpenHat:
            {
                v.baseFreq = 7500.0f;
                // Sizzling decay ~320ms
                v.env = 0.0f;
                v.noiseEnv = 1.0f;
                v.noiseDecay = std::exp(-1.0f / (0.32f * sr));
                v.filter.configure(BiquadFilter::Type::HighPass, 6000.0f, 1.0f, sr);
                break;
            }

            default:
                break;
        }
    }

    void DrumSampler::renderBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
    {
        if (!isEnabled)
            return;

        const float twoPi = 2.0f * static_cast<float>(std::numbers::pi);
        const float invSr = 1.0f / static_cast<float>(currentSampleRate);

        for (int s = 0; s < numSamples; ++s)
        {
            float mixedSample = 0.0f;

            for (auto& v : voices)
            {
                if (!v.active)
                    continue;

                float sampleOut = 0.0f;

                if (v.pad == DrumPad::Kick)
                {
                    const float currentFreq = v.baseFreq + (v.startFreq - v.baseFreq) * v.pitchEnv;
                    v.phaseInc = currentFreq * twoPi * invSr;
                    v.phase += v.phaseInc;
                    if (v.phase >= twoPi) v.phase -= twoPi;

                    const float sine = std::sin(v.phase);
                    const float click = v.filter.processSample(getWhiteNoise()) * v.noiseEnv;

                    // Soft saturation
                    sampleOut = std::tanh((sine * v.env * 1.2f) + click) * v.velocity;

                    v.env *= v.envDecay;
                    v.pitchEnv *= v.pitchDecay;
                    v.noiseEnv *= v.noiseDecay;

                    if (v.env < 0.001f && v.noiseEnv < 0.001f)
                        v.active = false;
                }
                else if (v.pad == DrumPad::Snare)
                {
                    const float currentFreq = v.baseFreq + (v.startFreq - v.baseFreq) * v.pitchEnv;
                    v.phaseInc = currentFreq * twoPi * invSr;
                    v.phase += v.phaseInc;
                    if (v.phase >= twoPi) v.phase -= twoPi;

                    const float body = std::sin(v.phase) * v.env * 0.7f;
                    const float noise = v.filter.processSample(getWhiteNoise()) * v.noiseEnv;

                    sampleOut = std::tanh(body + noise) * v.velocity;

                    v.env *= v.envDecay;
                    v.pitchEnv *= v.pitchDecay;
                    v.noiseEnv *= v.noiseDecay;

                    if (v.env < 0.001f && v.noiseEnv < 0.001f)
                        v.active = false;
                }
                else if (v.pad == DrumPad::ClosedHat || v.pad == DrumPad::OpenHat)
                {
                    const float noise = v.filter.processSample(getWhiteNoise()) * v.noiseEnv;
                    sampleOut = noise * v.velocity * 0.8f;

                    v.noiseEnv *= v.noiseDecay;

                    if (v.noiseEnv < 0.001f)
                        v.active = false;
                }

                mixedSample += sampleOut;
            }

            mixedSample *= masterGain;

            // Mix into all active output audio channels (typically Stereo L/R)
            const int currentSampleIdx = startSample + s;
            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            {
                outputBuffer.addSample(ch, currentSampleIdx, mixedSample);
            }
        }
    }
}

