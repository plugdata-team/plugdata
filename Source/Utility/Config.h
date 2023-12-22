#pragma once

#include <juce_data_structures/juce_data_structures.h>

using namespace juce;

#include "Utility/HashUtils.h"
#include "Utility/SynchronousValue.h"

namespace juce {
class AudioDeviceManager;
}
class MidiDeviceManager;
class PlugDataWindow;
class PluginEditor;
class StandalonePluginHolder;

struct ProjectInfo {

    static char const* projectName;
    static bool isStandalone;

    static inline char const* companyName = "plugdata";
    static inline char const* versionString = PLUGDATA_VERSION;
    static inline int const versionNumber = 0x800;

    static MidiDeviceManager* getMidiDeviceManager();
    static AudioDeviceManager* getDeviceManager();

    static PlugDataWindow* createNewWindow(PluginEditor* editor);
    static void closeWindow(PlugDataWindow* window);

    static StandalonePluginHolder* getStandalonePluginHolder();

    static bool isMidiEffect() noexcept;
    static bool canUseSemiTransparentWindows();

    static inline File const appDataDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata");

    static inline String const versionSuffix = "-2";
    static inline File const versionDataDir = appDataDir.getChildFile("Versions").getChildFile(ProjectInfo::versionString + versionSuffix);
};

template<typename T>
inline T getValue(Value const& v)
{
    if constexpr (std::is_same_v<T, String>) {
        return v.toString();
    } else {
        return static_cast<T>(v.getValue());
    }
}

inline void setValueExcludingListener(Value& parameter, var const& value, Value::Listener* listener)
{
    // This is only gonna work well on synchronous values!
    jassert(dynamic_cast<SynchronousValueSource*>(&parameter.getValueSource()) != nullptr);

    parameter.removeListener(listener);

    auto oldValue = parameter.getValue();
    parameter.setValue(value);

    parameter.addListener(listener);
}

static inline String convertURLtoUTF8(String const& input)
{
    StringArray tokens;
    tokens.addTokens(input, " ", "");
    String output;

    for (int i = 0; i < tokens.size(); ++i) {
        String token = tokens[i];

        if (token.startsWithChar('#')) {
            // Extract the hex value and convert it to a character
            auto hexString = token.substring(1);
            int hexValue;
            sscanf(hexString.toRawUTF8(), "%x", &hexValue);
            output += String::charToString(static_cast<wchar_t>(hexValue));
            output += token.substring(3);
        } else {
            output += token;
        }

        if (i < tokens.size() - 1) {
            output += " ";
        }
    }

    return output.trimEnd();
}

static inline String getRelativeTimeDescription(String const& timestampString)
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

    int years = static_cast<int>(relativeTime.inDays() / 365);
    int months = static_cast<int>(relativeTime.inDays() / 30);
    int weeks = static_cast<int>(relativeTime.inWeeks());
    int days = static_cast<int>(relativeTime.inDays());

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
