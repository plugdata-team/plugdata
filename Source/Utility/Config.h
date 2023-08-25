#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "Utility/HashUtils.h"

namespace juce {
class AudioDeviceManager;
}
class MidiDeviceManager;

using namespace juce;

struct ProjectInfo {

    static char const* projectName;
    static bool isStandalone;

    static inline char const* companyName = "plugdata";
    static inline char const* versionString = PLUGDATA_VERSION;
    static inline int const versionNumber = 0x800;

    static MidiDeviceManager* getMidiDeviceManager();
    static AudioDeviceManager* getDeviceManager();

    static bool canUseSemiTransparentWindows();
    
    static inline const File appDataDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata");
    
    static inline const String versionSuffix = "-7";
    static inline const File versionDataDir = appDataDir.getChildFile("Versions").getChildFile(ProjectInfo::versionString + versionSuffix);
};

template<typename T>
inline T getValue(Value const& v)
{
    return static_cast<T>(v.getValue());
}

static String getRelativeTimeDescription(const String& timestampString)
{
    StringArray dateAndTime = StringArray::fromTokens(timestampString, true);
    StringArray dateComponents = StringArray::fromTokens(dateAndTime[0], "-", "");

    int year = dateComponents[0].getIntValue();
    int month = dateComponents[1].getIntValue();
    int day = dateComponents[2].getIntValue();

    StringArray timeComponents = StringArray::fromTokens(dateAndTime[1], ":", "");
    int hour = timeComponents[0].getIntValue();
    int minute = timeComponents[1].getIntValue();
    int second = timeComponents[2].getIntValue();

    Time timestamp(year, month, day, hour, minute, second, 0);
    Time currentTime = Time::getCurrentTime();
    RelativeTime relativeTime = currentTime - timestamp;

    
    int years = relativeTime.inDays() / 365;
    int months = relativeTime.inDays() / 30;
    int weeks = relativeTime.inWeeks();
    int days = relativeTime.inDays();

    if (years == 1)
        return String(years) + " year ago";
    else if (years > 0)
        return String(years) + " years ago";
    else if (months == 1)
        return String(months) + " month ago";
    else if (months > 0)
        return String(months) + " months ago";
    else if (weeks == 1)
        return String(weeks) + " week ago";
    else if (weeks > 0)
        return String(weeks) + " weeks ago";
    else if (days == 1)
        return String(days) + " day ago";
    else if (days > 0)
        return String(days) + " days ago";
    else
        return "today";
}
