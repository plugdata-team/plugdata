# PlugData
Pure Data as a plugin, with a new GUI

<img width="948" alt="Screenshot 2021-11-30 at 20 06 32" src="https://user-images.githubusercontent.com/44585538/144111560-4907f0a1-afb7-452e-b3f0-e69ac98128ce.png">




PlugData a plugin wrapper for PureData, featuring a new GUI made with JUCE. It can still use some stability fixes, especially for complicated patches. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs.

Possible thanks to:

- Camomile by Pierre Guillot
- Pure Data by Miller Puckette and others
- libpd by the Peter Brinkmann, Dan Wilcox and others
- ELSE by Alexandre Torres Porres
- Juce by ROLI Ltd.
- MoodyCamel by Cameron Desrochers
- VST PlugIn Technology by Steinberg Media Technologies
- Audio Unit PlugIn Technology by Apple

What works:
- Creating, deleting and moving objects and connections
- Signal and data objects
- Most GUI objects:
  - Bang, Numbox, Toggle, Radio, Sliders, Array/Graph and Message work perfectly
  - Fairly good support for GraphOnParent and subpatchers (could use more testing)
- Usable in DAW
- Console
- Saving and loading patches
- Undoing, redoing, copying, pasting, duplicating
- GUI where locking is optional, all components have headers. There is a option to set it to headerless mode to make native pd patches look better.
- Automatic object name completion

Known issues:
- Some abstractions from ELSE cause crashes
- There may still be some other stability problems

Future plans:
- Add DAW automation as receive

Please contact me if you wish to contribute, I could use some help!
