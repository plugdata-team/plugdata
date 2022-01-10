
ff_meters
=========

by Daniel Walz / Foleys Finest Audio Ltd.
Published under the BSD License (3 clause)

The ff_meters provide an easy to use Component to display a level reading for an
AudioBuffer. It is to be used in the audio framework JUCE (www.juce.com).

Find the API documentation here: https://ffaudio.github.io/ff_meters/


Usage
=====

LevelMeter
----------

To use it create a LevelMeterSource instance next to the AudioBuffer you want to
display. To update the meter, call LevelMeterSource::measureBlock (buffer) in your
processBlock or getNextAudioBuffer method.

On the Component use LevelMeter::setMeterSource to link to the LevelMeterSource 
instance. The number of channels will be updated automatically.

You can pull the drawing into your LookAndFeel by inheriting LevelMeter::LookAndFeelMethods
and inlining the default implementation from LevelMeterLookAndFeel in 
ff_meters_LookAndFeelMethods.h into a public section of your class declaration. To
setup the default colour scheme, call setupDefaultMeterColours() in your constructor.

Or you can use the LevelMeterLookAndFeel directly because it inherits from juce::LookAndFeel_V3 
for your convenience. You can set it as default LookAndFeel, if you used the default, 
or set it only to the meters, if you don't want it to interfere.

All classes are in the namespace `foleys` to avoid collisions. You can either prefix each symbol, 
or import the namespace.
N.B. for backward compatibility, `FFAU` is an alias for `foleys`.

    // In your Editor
    public:
        PluginEditor()
        {
            // adjust the colours to how you like them, e.g.
            lnf.setColour (foleys::LevelMeter::lmMeterGradientLowColour, juce::Colours::green);
    
            meter.setLookAndFeel (&lnf);
            meter.setMeterSource (&processor.getMeterSource());
            addAndMakeVisible (meter);
            // ...
        }
        ~PluginEditor()
        {
            meter.setLookAndFeel (nullptr);
        }

    private:
        foleys::LevelMeterLookAndFeel lnf;
        foleys::LevelMeter meter { foleys::LevelMeter::Minimal }; // See foleys::LevelMeter::MeterFlags for options

    // and in the processor:
    public:
        foleys::LevelMeterSource& getMeterSource()
        {
            return meterSource;
        }

        void prepareToPlay (double sampleRate, int samplesPerBlockExpected) override
        {
            // this prepares the meterSource to measure all output blocks and average over 100ms to allow smooth movements
            meterSource.resize (getTotalNumOutputChannels(), sampleRate * 0.1 / samplesPerBlockExpected);
            // ...
        }
        void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
        {
            meterSource.measureBlock (buffer);
            // ...
        }

    private:
        foleys::LevelMeterSource meterSource;


OutlineBuffer
-------------

Another class is capable of reducing the samples going through into min and max blocks. This
way you can see the outline of a signal running through. It can be used very similar:

    // in your processor
    private:
    foleys::OutlineBuffer outline;

    // in prepareToPlay
    outline.setSize (getTotalNumInputChannels(), 1024);

    // in processBlock
    outline.pushBlock (buffer, buffer.getNumSamples());

    // and in the editor's component:
    const Rectangle<float> plotFrame (10.0f, 320.0f, 580f, 80f);
    g.setColour (Colours::lightgreen);
    g.fillRect (plotFrame);

    Path plot;
    processor.getChannelOutline (plot, plotFrame, 1000);
    g.setColour (Colours::grey);
    g.fillPath (plot);
    g.setColour (Colours::black);
    g.strokePath (plot, PathStrokeType (1.0f));


********************************************************************************

We hope it is of any use, let us know of any problems or improvements you may 
come up with...

Brighton, 2nd March 2017

********************************************************************************
