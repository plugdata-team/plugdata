# PlugData
Pure Data as a plugin, with a new GUI

<img width="948" alt="Screenshot 2021-11-29 at 20 29 57" src="https://user-images.githubusercontent.com/44585538/143930579-2af7e038-4dd9-4c71-9b02-2c9f16c13538.png">



PlugData a plugin wrapper for PureData, featuring a new GUI made with JUCE. Currently not very stable!
This project differs from Camomile in that it actually allows you to modify patches

Possible thanks to:

- Camomile by Pierre Guillot
- Pure Data by Miller Puckette and others
- libpd by the Peter Brinkmann, Dan Wilcox and others
- Juce by ROLI Ltd.
- MoodyCamel by Cameron Desrochers
- VST PlugIn Technology by Steinberg Media Technologies
- Audio Unit PlugIn Technology by Apple

What works:
- Creating, deleting and moving objects and connections
- Signal and data objects
- Most GUI objects:
  - Bang, Numbox, Toggle, Radio, Sliders, Array/Graph and Message work perfectly
  - GraphOnParent and VUMeter do not yet work
- Usable in DAW
- Console
- Saving and loading patches
- Undoing and redoing


What doesn't work / needs to be fixed:
- Perhaps entirely thread-safe yet
- Some hacky solutions
- No VU meter
- GraphOnParent and Subpatches not supported yet


Please contact me if you wish to contribute, I could use some help!
