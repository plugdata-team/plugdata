# PlugData
Pure Data as a plugin, with a new GUI


PlugData a plugin wrapper for PureData, featuring a new GUI made with JUCE. Currently not very stable!

Possible thanks to:

Camomile by Pierre Guillot
Pure Data by Miller Puckette and others
libpd by the Peter Brinkmann, Dan Wilcox and others
Juce by ROLI Ltd.
MoodyCamel by Cameron Desrochers
VST PlugIn Technology by Steinberg Media Technologies
Audio Unit PlugIn Technology by Apple

What works:
- Creating, deleting and moving objects
- Signal and data objects
- Most GUI objects:
  - Bang, Numbox, Toggle, Radio, Sliders, Array/Graph work perfectly
  - Message boxes almost work, but have some quirks
  - GraphOnParent does not yet work
- Usable in DAW
- Console

What kinda works (but needs testing):
- Saving and loading patches
- Message boxes

What doesn't work / needs to be fixed:
- Not entirely thread-safe yet
- Undo/Redo
- Some hacky solutions

Please contact me if you wish to contribute, I could use some help!
