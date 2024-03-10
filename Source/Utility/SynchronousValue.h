#pragma once

class SynchronousValueSource : public Value::ValueSource {
public:
    SynchronousValueSource()
    {
    }

    explicit SynchronousValueSource(var const& initialValue)
        : value(initialValue)
    {
    }

    var getValue() const override
    {
        return value;
    }

    void setValue(var const& newValue) override
    {
        if (!newValue.equalsWithSameType(value)) {
            value = newValue;
            sendChangeMessage(true);
        }
    }

private:
    var value;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynchronousValueSource)
};

class ThreadedSynchronousValueSource : public Value::ValueSource {
public:
    ThreadedSynchronousValueSource()
    {
    }

    explicit ThreadedSynchronousValueSource(var const& initialValue)
        : value(initialValue)
    {
    }

    var getValue() const override
    {
        ScopedLock lock(cs);
        return value;
    }

    void setValue(var const& newValue) override
    {
        if (!newValue.equalsWithSameType(value)) {
            {
                ScopedLock lock(cs);
                value = newValue;
            }
            sendChangeMessage(true);
        }
    }

private:
    var value;
    CriticalSection cs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreadedSynchronousValueSource)
};

#define ThreadSafeValue(x) Value(new ThreadedSynchronousValueSource(x))

#define SynchronousValue(x) Value(new SynchronousValueSource(x))
