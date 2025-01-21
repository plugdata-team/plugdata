#include <juce_gui_basics/juce_gui_basics.h>
#include <random>

#include "Utility/Config.h"
#include "Utility/OSUtils.h"
#include "Sidebar/Sidebar.h" // So we can read and clear the console
#include "Objects/ObjectBase.h" // So we can interact with object GUIs
#include "PluginEditor.h"

class PlugDataUnitTest : public UnitTest, public AsyncUpdater
{
protected:
    PlugDataUnitTest(PluginEditor* editor, String const& name) : UnitTest(name), editor(editor)
    {
    }

    virtual void perform() = 0;

    void signalDone()
    {
        testDone.signal();
    }

    // Generate a random float
    float generateRandomFloat()
    {
        return fmod(getRandom().nextFloat(), 10.f);
    }

    pd::Atom generateIncorrectArg()
    {
        return generateRandomFloat() < 10.0f ? pd::Atom(generateRandomFloat()) : pd::Atom(generateRandomString(getRandom().nextInt() + 5));
    }

    t_symbol* generateRandomString(size_t length)
    {
        const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string result;
        for (size_t i = 0; i < length; ++i)
        {
            result += charset[getRandom().nextInt(sizeof(charset) - 2)];
        }
        return gensym(result.c_str());
    }

    // Generate a random pd::Atom based on type
    SmallArray<pd::Atom> generateAtomForType(const std::string& type)
    {
        if (type == "<float>" || type == "<f>")
        {
            return {pd::Atom(generateRandomFloat())};
        }
        else if (type == "<float, float>" || type == "<f, f>")
        {
            // Make second float higher, in case it's a range
            auto rand1 = generateRandomFloat();
            return {pd::Atom(rand1), pd::Atom(rand1 + generateRandomFloat())};
        }
        else if (type == "<symbol>")
        {
            return {pd::Atom(generateRandomString(10))};
        }
        else if (type == "<int>")
        {
            return {pd::Atom(static_cast<int>(rand() % 12))}; // Random integer
        }
        else if (type == "<bool>")
        {
            return {pd::Atom(static_cast<int>(rand() % 2))}; // Random boolean (as 0 or 1)
        }
        else
        {
            // Default to a random float for unrecognized types
            return {pd::Atom(generateRandomFloat())};
        }
    }

private:

    void runTest() final
    {
        triggerAsyncUpdate();
        testDone.wait();
    }

    void handleAsyncUpdate() final
    {
        perform();
    }

    WaitableEvent testDone;

protected:
    PluginEditor* editor;
};
