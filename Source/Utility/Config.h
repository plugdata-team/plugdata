#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

using namespace juce;

#include "Utility/Containers.h"
#include "Utility/Hash.h"
#include "Utility/SynchronousValue.h"
#include "Utility/SeqLock.h"
#include "Utility/OSUtils.h"

namespace juce {
class AudioDeviceManager;
}
class PlugDataWindow;
class PluginEditor;
class StandalonePluginHolder;

struct ProjectInfo {

    static char const* projectName;
    static bool isStandalone;
    static bool isFx;

    static inline char const* companyName = "plugdata";
    static inline char const* versionString = PLUGDATA_VERSION;

    static AudioDeviceManager* getDeviceManager();

    static PlugDataWindow* createNewWindow(PluginEditor* editor);
    static void closeWindow(PlugDataWindow* window);

    static StandalonePluginHolder* getStandalonePluginHolder();

    static bool isMidiEffect() noexcept;
    static bool canUseSemiTransparentWindows();

#if JUCE_WINDOWS
    // Regular documents directory might be synced to OneDrive
    static inline File const appDataDir = File::getSpecialLocation(File::SpecialLocationType::commonDocumentsDirectory).getChildFile("plugdata");
#elif JUCE_IOS
    static inline File const appDataDir = File::getContainerForSecurityApplicationGroupIdentifier("group.com.plugdata.plugdata");
#else
    static inline File const appDataDir = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile("plugdata");
#endif
    static inline String const versionSuffix = "-18";
    static inline File const versionDataDir = appDataDir.getChildFile("Versions").getChildFile(ProjectInfo::versionString + versionSuffix);
};

template<typename T>
T getValue(Value const& v)
{
    if constexpr (std::is_same_v<T, String>) {
        return v.toString();
    } else if constexpr (std::is_same_v<T, Colour>) {
        return Colour::fromString(v.toString());
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

inline String convertURLtoUTF8(String const& input)
{
    StringArray tokens;
    tokens.addTokens(input, " ", "");
    String output;

    for (int i = 0; i < tokens.size(); ++i) {
        String const& token = tokens[i];

        if (token.startsWithChar('#')) {
            // Extract the hex value and convert it to a character
            auto hexString = token.substring(1) + "\0";
            char* endptr;
            int const hexValue = strtoul(hexString.toRawUTF8(), &endptr, 16);
            if (*endptr == '\0') {
                output += String::charToString(static_cast<wchar_t>(hexValue));
                output += token.substring(3);
            } else {
                jassertfalse;
                output += token;
            }
        } else {
            output += token;
        }

        if (i < tokens.size() - 1) {
            output += " ";
        }
    }

    return output.trimEnd();
}

inline String getRelativeTimeDescription(String const& timestampString)
{
    StringArray const dateAndTime = StringArray::fromTokens(timestampString, true);
    StringArray const dateComponents = StringArray::fromTokens(dateAndTime[0], "-", "");

    int const year = dateComponents[0].getIntValue();
    int const month = dateComponents[1].getIntValue();
    int const day = dateComponents[2].getIntValue();

    StringArray const timeComponents = StringArray::fromTokens(dateAndTime[1], ":", "");
    int const hour = timeComponents[0].getIntValue();
    int const minute = timeComponents[1].getIntValue();
    int const second = timeComponents[2].getIntValue();

    Time const timestamp(year, month, day, hour, minute, second, 0);
    Time const currentTime = Time::getCurrentTime();
    RelativeTime const relativeTime = currentTime - timestamp;

    int const years = static_cast<int>(relativeTime.inDays() / 365);
    int const months = static_cast<int>(relativeTime.inDays() / 30);
    int const weeks = static_cast<int>(relativeTime.inWeeks());
    int const days = static_cast<int>(relativeTime.inDays());

    if (years == 1)
        return String(years) + " year ago";
    if (years > 0)
        return String(years) + " years ago";
    if (months == 1)
        return String(months) + " month ago";
    if (months > 0)
        return String(months) + " months ago";
    if (weeks == 1)
        return String(weeks) + " week ago";
    if (weeks > 0)
        return String(weeks) + " weeks ago";
    if (days == 1)
        return String(days) + " day ago";
    if (days > 0)
        return String(days) + " days ago";

    return "today";
}

// Allow hashing weak references
template<typename T>
struct std::hash<juce::WeakReference<T>> {
    std::size_t operator()(juce::WeakReference<T> const& key) const
    {
        auto const ptr = reinterpret_cast<std::size_t>(key.get());
        return ptr ^ ptr >> 16; // Simple XOR-shift for better distribution
    }
};
