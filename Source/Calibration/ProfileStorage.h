#pragma once

#include "../Classifier/ModelData.h"
#include <juce_core/juce_core.h>

namespace TeethDrummer
{
    class ProfileStorage
    {
    public:
        static juce::String serializeProfile(const UserProfile& profile);
        static bool deserializeProfile(const juce::String& jsonString, UserProfile& outProfile);

        static bool saveToFile(const UserProfile& profile, const juce::File& file);
        static bool loadFromFile(const juce::File& file, UserProfile& outProfile);
    };
}

