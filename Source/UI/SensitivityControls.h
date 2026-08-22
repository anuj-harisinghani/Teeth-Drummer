#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace TeethDrummer
{
    class SensitivityControls : public juce::Component
    {
    public:
        SensitivityControls(juce::AudioProcessorValueTreeState& apvts)
        {
            auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text, const juce::String& suffix = "") {
                s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 16);
                s.setTextValueSuffix(suffix);
                addAndMakeVisible(s);

                l.setText(text, juce::dontSendNotification);
                l.setFont(juce::Font(12.0f, juce::Font::bold));
                l.setJustificationType(juce::Justification::centred);
                l.setColour(juce::Label::textColourId, juce::Colour(0xffd1d5db));
                addAndMakeVisible(l);
            };

            setupSlider(thresholdSlider, thresholdLabel, "Threshold");
            thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "threshold", thresholdSlider);

            setupSlider(sensitivitySlider, sensitivityLabel, "Sensitivity");
            sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "sensitivity", sensitivitySlider);

            setupSlider(noiseGateSlider, noiseGateLabel, "Noise Gate", " dB");
            noiseGateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "noiseGate", noiseGateSlider);

            setupSlider(retriggerSlider, retriggerLabel, "Retrigger", " ms");
            retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "retriggerMs", retriggerSlider);

            setupSlider(velSensSlider, velSensLabel, "Vel Gain");
            velSensAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "velSensitivity", velSensSlider);

            setupSlider(velCurveSlider, velCurveLabel, "Vel Curve");
            velCurveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "velCurve", velCurveSlider);

            setupSlider(synthVolSlider, synthVolLabel, "Preview Vol", " dB");
            synthVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, "synthVolume", synthVolSlider);

            synthToggle.setButtonText("Audio Preview");
            synthToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffe5e7eb));
            addAndMakeVisible(synthToggle);
            synthToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, "synthEnabled", synthToggle);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            const int numSliders = 7;
            const int sliderWidth = bounds.getWidth() / numSliders;

            auto placeSlider = [&](juce::Slider& s, juce::Label& l, juce::Rectangle<int> rect) {
                l.setBounds(rect.removeFromTop(20));
                s.setBounds(rect);
            };

            placeSlider(thresholdSlider, thresholdLabel, bounds.removeFromLeft(sliderWidth));
            placeSlider(sensitivitySlider, sensitivityLabel, bounds.removeFromLeft(sliderWidth));
            placeSlider(noiseGateSlider, noiseGateLabel, bounds.removeFromLeft(sliderWidth));
            placeSlider(retriggerSlider, retriggerLabel, bounds.removeFromLeft(sliderWidth));
            placeSlider(velSensSlider, velSensLabel, bounds.removeFromLeft(sliderWidth));
            placeSlider(velCurveSlider, velCurveLabel, bounds.removeFromLeft(sliderWidth));
            placeSlider(synthVolSlider, synthVolLabel, bounds.removeFromLeft(sliderWidth));
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0xff18181f));
            g.fillRoundedRectangle(bounds, 6.0f);
            g.setColour(juce::Colour(0xff2a2a38));
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        }

    private:
        juce::Slider thresholdSlider, sensitivitySlider, noiseGateSlider, retriggerSlider;
        juce::Slider velSensSlider, velCurveSlider, synthVolSlider;
        juce::Label thresholdLabel, sensitivityLabel, noiseGateLabel, retriggerLabel;
        juce::Label velSensLabel, velCurveLabel, synthVolLabel;
        juce::ToggleButton synthToggle;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensitivityAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseGateAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retriggerAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velSensAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velCurveAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> synthVolAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> synthToggleAttachment;
    };
}

