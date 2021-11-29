# PlugData
Pure Data as a plugin, with a new GUI


<img width="948" alt="Screenshot 2021-11-29 at 20 34 14" src="https://user-images.githubusercontent.com/44585538/143931001-6fbb2975-e96c-4eab-ac4e-6ae0da183735.png">


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
- Perhaps not entirely thread-safe yet
- Some hacky solutions
- No VU meter
- GraphOnParent and Subpatches not supported yet


Please contact me if you wish to contribute, I could use some help!
