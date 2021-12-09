# PlugData
Pure Data as a plugin, with a new GUI


<img width="1124" alt="Screenshot 2021-12-07 at 02 19 24" src="https://user-images.githubusercontent.com/44585538/144948333-97e9b1ea-c323-46c1-8d7e-abfcc0b569f8.png">



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
- Some objects crash on native Apple Silicon (I believe this is a pd issue), it runs fine in Rosetta
- There may still be some other stability problems
- Very large patches are slow (try All_objects from ELSE)

Future plans:
- Add DAW automation as receive

Please contact me if you wish to contribute, I could use some help!
