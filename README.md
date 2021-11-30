# PlugData
Pure Data as a plugin, with a new GUI

<img width="948" alt="Screenshot 2021-11-30 at 20 05 37" src="https://user-images.githubusercontent.com/44585538/144111439-250dba13-4cf7-4689-8efc-00b27986426a.png">




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
  - Basic support for GraphOnParent
- Usable in DAW
- Console
- Saving and loading patches
- Undoing and redoing


What doesn't work / needs to be fixed:
- No VU meter
- Cannot open/edit subpatches or abstractions
- Possibly some thread-safety issues, and some slightly hacky manipulation of pd


Please contact me if you wish to contribute, I could use some help!
