#include "ProfileStorage.h"

namespace TeethDrummer
{
    juce::String ProfileStorage::serializeProfile(const UserProfile& profile)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("profileName", profile.profileName);

        auto* padsArray = new juce::Array<juce::var>();
        for (const auto& proto : profile.prototypes)
        {
            auto* padObj = new juce::DynamicObject();
            padObj->setProperty("pad", static_cast<int>(proto.pad));
            padObj->setProperty("padName", juce::String(getDrumPadName(proto.pad).data()));
            padObj->setProperty("isCalibrated", proto.isCalibrated);
            padObj->setProperty("sampleCount", proto.sampleCount);
            padObj->setProperty("lowEnergyRatio", proto.lowEnergyRatio);
            padObj->setProperty("midEnergyRatio", proto.midEnergyRatio);
            padObj->setProperty("highEnergyRatio", proto.highEnergyRatio);
            padObj->setProperty("spectralCentroid", proto.spectralCentroid);
            padObj->setProperty("zeroCrossingRate", proto.zeroCrossingRate);
            padObj->setProperty("decaySlope", proto.decaySlope);

            padsArray->add(juce::var(padObj));
        }

        obj->setProperty("prototypes", juce::var(padsArray));
        return juce::JSON::toString(juce::var(obj), false);
    }

    bool ProfileStorage::deserializeProfile(const juce::String& jsonString, UserProfile& outProfile)
    {
        auto parsed = juce::JSON::parse(jsonString);
        if (!parsed.isObject())
            return false;

        auto* obj = parsed.getDynamicObject();
        if (obj == nullptr)
            return false;

        if (obj->hasProperty("profileName"))
            outProfile.profileName = obj->getProperty("profileName").toString().toStdString();

        auto protosVar = obj->getProperty("prototypes");
        if (protosVar.isArray())
        {
            auto* arr = protosVar.getArray();
            for (int i = 0; i < arr->size(); ++i)
            {
                auto& item = arr->getReference(i);
                if (item.isObject())
                {
                    auto* pObj = item.getDynamicObject();
                    int padInt = static_cast<int>(pObj->getProperty("pad"));
                    if (padInt >= 0 && padInt < static_cast<int>(DrumPad::Count))
                    {
                        auto& proto = outProfile.prototypes[static_cast<size_t>(padInt)];
                        proto.pad = static_cast<DrumPad>(padInt);
                        proto.isCalibrated   = static_cast<bool>(pObj->getProperty("isCalibrated"));
                        proto.sampleCount    = static_cast<int>(pObj->getProperty("sampleCount"));
                        proto.lowEnergyRatio = static_cast<float>(pObj->getProperty("lowEnergyRatio"));
                        proto.midEnergyRatio = static_cast<float>(pObj->getProperty("midEnergyRatio"));
                        proto.highEnergyRatio= static_cast<float>(pObj->getProperty("highEnergyRatio"));
                        proto.spectralCentroid = static_cast<float>(pObj->getProperty("spectralCentroid"));
                        proto.zeroCrossingRate = static_cast<float>(pObj->getProperty("zeroCrossingRate"));
                        proto.decaySlope     = static_cast<float>(pObj->getProperty("decaySlope"));
                    }
                }
            }
        }

        return true;
    }

    bool ProfileStorage::saveToFile(const UserProfile& profile, const juce::File& file)
    {
        const auto json = serializeProfile(profile);
        return file.replaceWithText(json);
    }

    bool ProfileStorage::loadFromFile(const juce::File& file, UserProfile& outProfile)
    {
        if (!file.existsAsFile())
            return false;

        const auto json = file.loadFileAsString();
        return deserializeProfile(json, outProfile);
    }
}

