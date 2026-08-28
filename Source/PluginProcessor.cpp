#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Calibration/ProfileStorage.h"

namespace TeethDrummer
{
    TeethDrummerAudioProcessor::TeethDrummerAudioProcessor()
        : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "Parameters", createParameterLayout()),
          calibrationManager(classifierEngine)
    {
    }

    TeethDrummerAudioProcessor::~TeethDrummerAudioProcessor()
    {
    }

    juce::AudioProcessorValueTreeState::ParameterLayout TeethDrummerAudioProcessor::createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // Detection sensitivity and thresholds
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"threshold", 1}, "Threshold",
            juce::NormalisableRange<float>(0.01f, 1.0f, 0.01f), 0.25f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"sensitivity", 1}, "Sensitivity",
            juce::NormalisableRange<float>(0.1f, 5.0f, 0.05f), 1.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"noiseGate", 1}, "Noise Gate dB",
            juce::NormalisableRange<float>(-80.0f, -10.0f, 0.5f), -50.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"retriggerMs", 1}, "Retrigger ms",
            juce::NormalisableRange<float>(10.0f, 100.0f, 1.0f), 30.0f));

        // Velocity translation
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"velSensitivity", 1}, "Velocity Sensitivity",
            juce::NormalisableRange<float>(0.2f, 4.0f, 0.05f), 1.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"velCurve", 1}, "Velocity Curve",
            juce::NormalisableRange<float>(0.2f, 3.0f, 0.05f), 0.8f));

        // Internal Preview Synth controls
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"synthEnabled", 1}, "Preview Sampler", true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"synthVolume", 1}, "Preview Volume",
            juce::NormalisableRange<float>(-60.0f, 6.0f, 0.5f), -3.0f));

        // MIDI Settings
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"midiChannel", 1}, "MIDI Channel", 1, 16, 10));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"noteKick", 1}, "Kick MIDI Note", 0, 127, DefaultMidiNotes::Kick));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"noteSnare", 1}, "Snare MIDI Note", 0, 127, DefaultMidiNotes::Snare));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"noteClosedHat", 1}, "Closed Hat MIDI Note", 0, 127, DefaultMidiNotes::ClosedHat));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"noteOpenHat", 1}, "Open Hat MIDI Note", 0, 127, DefaultMidiNotes::OpenHat));

        return { params.begin(), params.end() };
    }

    void TeethDrummerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        transientDetector.prepare(sampleRate, samplesPerBlock);
        featureExtractor.prepare(sampleRate);
        midiEngine.prepare(sampleRate);
        drumSampler.prepare(sampleRate);
        historyBuffer.reset();
        telemetryQueue.reset();
        scopeQueue.reset();
        scopeSubsampleCounter = 0;
    }

    void TeethDrummerAudioProcessor::releaseResources()
    {
        transientDetector.reset();
        featureExtractor.reset();
        midiEngine.reset();
        drumSampler.reset();
        historyBuffer.reset();
    }

    bool TeethDrummerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()
            && !layouts.getMainInputChannelSet().isDisabled())
            return false;

        return true;
    }

    void TeethDrummerAudioProcessor::updateParameters()
    {
        transientDetector.setThreshold(apvts.getRawParameterValue("threshold")->load());
        transientDetector.setSensitivity(apvts.getRawParameterValue("sensitivity")->load());
        
        const float gateDb = apvts.getRawParameterValue("noiseGate")->load();
        transientDetector.setNoiseGateFloor(juce::Decibels::decibelsToGain(gateDb));
        transientDetector.setRetriggerTimeMs(apvts.getRawParameterValue("retriggerMs")->load());

        midiEngine.getVelocityCurve().setSensitivity(apvts.getRawParameterValue("velSensitivity")->load());
        midiEngine.getVelocityCurve().setCurveExponent(apvts.getRawParameterValue("velCurve")->load());

        drumSampler.setEnabled(apvts.getRawParameterValue("synthEnabled")->load() > 0.5f);
        const float synthVolDb = apvts.getRawParameterValue("synthVolume")->load();
        drumSampler.setVolume(juce::Decibels::decibelsToGain(synthVolDb));

        midiEngine.setMidiChannel(static_cast<uint8_t>(apvts.getRawParameterValue("midiChannel")->load()));
        midiEngine.setPadNote(DrumPad::Kick, static_cast<uint8_t>(apvts.getRawParameterValue("noteKick")->load()));
        midiEngine.setPadNote(DrumPad::Snare, static_cast<uint8_t>(apvts.getRawParameterValue("noteSnare")->load()));
        midiEngine.setPadNote(DrumPad::ClosedHat, static_cast<uint8_t>(apvts.getRawParameterValue("noteClosedHat")->load()));
        midiEngine.setPadNote(DrumPad::OpenHat, static_cast<uint8_t>(apvts.getRawParameterValue("noteOpenHat")->load()));
    }

    void TeethDrummerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        juce::ScopedNoDenormals noDenormals;
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        updateParameters();

        // If no input audio channels are present, just render pending MIDI note-offs and synth
        if (numChannels == 0)
        {
            midiEngine.processBlock(numSamples, midiMessages);
            return;
        }

        const float* inL = buffer.getReadPointer(0);
        const float* inR = (numChannels > 1) ? buffer.getReadPointer(1) : inL;

        // Process audio sample by sample for sub-buffer onset timing
        for (int i = 0; i < numSamples; ++i)
        {
            // Sum input to mono for detection
            const float monoSample = (inL[i] + inR[i]) * 0.5f;

            // Push into circular history ring buffer
            historyBuffer.pushSample(monoSample);

            // Subsample for oscilloscope visualizer (push every 4th sample to conserve queue)
            if (++scopeSubsampleCounter >= 4)
            {
                scopeSubsampleCounter = 0;
                scopeQueue.push(monoSample);
            }

            // Check for transient onset
            float peakAmp = 0.0f;
            if (transientDetector.processSample(monoSample, peakAmp))
            {
                // 1. Grab recent window for feature extraction (128 samples = ~2.9ms @ 44.1k)
                historyBuffer.copyRecentSamples(analysisBuffer.data(), analysisBuffer.size());

                // 2. Extract acoustic features
                const FeatureVector feats = featureExtractor.extract(analysisBuffer.data(), analysisBuffer.size());

                // 3. Queue hit for calibration if active (lock-free, non-blocking)
                calibrationManager.queueCalibrationHit(feats);

                // 4. Classify drum hit
                float confidence = 0.0f;
                const DrumPad detectedPad = classifierEngine.classify(feats, confidence);

                if (detectedPad != DrumPad::None)
                {
                    // 5. Dispatch sample-accurate MIDI Note-On
                    const uint8_t midiVel = midiEngine.triggerHit(detectedPad, peakAmp, i, midiMessages);

                    // 6. Trigger internal preview drum sampler
                    const float normVel = static_cast<float>(midiVel) / 127.0f;
                    drumSampler.trigger(detectedPad, normVel);

                    // 7. Push telemetry event to GUI thread
                    TriggerTelemetryEvent event;
                    event.pad = detectedPad;
                    event.velocity = normVel;
                    event.midiVelocity = midiVel;
                    event.peakMagnitude = peakAmp;
                    event.spectralCentroid = feats.spectralCentroid;
                    event.lowEnergyRatio = feats.lowEnergyRatio;
                    event.midEnergyRatio = feats.midEnergyRatio;
                    event.highEnergyRatio = feats.highEnergyRatio;
                    event.confidence = confidence;
                    event.sampleTimestamp = i;

                    telemetryQueue.push(event);
                }
            }
        }

        // Process scheduled Note-Off events
        midiEngine.processBlock(numSamples, midiMessages);

        // Clear output buffer if not passing through input or synthesize drums
        buffer.clear();
        drumSampler.renderBlock(buffer, 0, numSamples);
    }

    void TeethDrummerAudioProcessor::triggerManualPad(DrumPad pad, float velocity)
    {
        drumSampler.trigger(pad, velocity);
    }

    const juce::String TeethDrummerAudioProcessor::getName() const { return "Teeth Drummer"; }
    bool TeethDrummerAudioProcessor::acceptsMidi() const { return false; }
    bool TeethDrummerAudioProcessor::producesMidi() const { return true; }
    bool TeethDrummerAudioProcessor::isMidiEffect() const { return false; }
    double TeethDrummerAudioProcessor::getTailLengthSeconds() const { return 0.0; }
    int TeethDrummerAudioProcessor::getNumPrograms() { return 1; }
    int TeethDrummerAudioProcessor::getCurrentProgram() { return 0; }
    void TeethDrummerAudioProcessor::setCurrentProgram(int) {}
    const juce::String TeethDrummerAudioProcessor::getProgramName(int) { return {}; }
    void TeethDrummerAudioProcessor::changeProgramName(int, const juce::String&) {}

    void TeethDrummerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
    {
        auto state = apvts.copyState();
        auto profileJson = ProfileStorage::serializeProfile(classifierEngine.getProfile());
        state.setProperty("calibratedProfileJson", profileJson, nullptr);

        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void TeethDrummerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
    {
        std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
        if (xmlState.get() != nullptr && xmlState->hasTagName(apvts.state.getType()))
        {
            auto valueTree = juce::ValueTree::fromXml(*xmlState);
            apvts.replaceState(valueTree);

            if (valueTree.hasProperty("calibratedProfileJson"))
            {
                juce::String json = valueTree.getProperty("calibratedProfileJson");
                UserProfile loadedProfile;
                if (ProfileStorage::deserializeProfile(json, loadedProfile))
                {
                    classifierEngine.setProfile(loadedProfile);
                }
            }
        }
    }

    bool TeethDrummerAudioProcessor::hasEditor() const { return true; }
    juce::AudioProcessorEditor* TeethDrummerAudioProcessor::createEditor()
    {
        return new TeethDrummerAudioProcessorEditor(*this);
    }
}

// Global JUCE factory export
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TeethDrummer::TeethDrummerAudioProcessor();
}

