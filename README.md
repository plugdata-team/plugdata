# PlugData
Pure Data as a plugin, with a new GUI

<img width="1007" alt="Screenshot 2021-11-28 at 20 06 01" src="https://user-images.githubusercontent.com/44585538/143782272-858d8b40-ccb0-4ed7-81aa-08709cc9d472.png">


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
- Creating, deleting and moving objects
- Signal and data objects
- Most GUI objects:
  - Bang, Numbox, Toggle, Radio, Sliders, Array/Graph and Message work perfectly
  - GraphOnParent does not yet work
- Usable in DAW
- Console

What kinda works (but needs testing):
- Saving and loading patches

What doesn't work / needs to be fixed:
- Not entirely thread-safe yet
- Undo/Redo
- Some hacky solutions
- Leaks
- No VU meter
- GraphOnParent
- Subpatches

Please contact me if you wish to contribute, I could use some help!
