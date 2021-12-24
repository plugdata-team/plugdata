# PlugData
Pure Data as a plugin, with a new GUI

<img width="1036" alt="Screenshot 2021-12-16 at 19 48 15" src="https://user-images.githubusercontent.com/44585538/146430994-594bb591-83e0-4da1-9bae-49e62054bef1.png">



PlugData a plugin wrapper for PureData, featuring a new GUI made with JUCE. It can still use some stability fixes, especially for complicated patches. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs. It can also be used as a standalone replacement for pure-data.

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
- Very close to full support for pd (including almost all GUI objects, undo/redo, copy/paste, saving, loading, console, setting object properties, Pd's drawing functions)
- Most ELSE library objects
- Receive 8 DAW parameters by using "receive param1"


Known issues:
- There may still be some bugs
- Can be slow for very large patches
- No VU meter, no canvas object, a few missing GUI objects from ELSE

Currently PlugData is feature-frozen until all bugs are fixed and the code is cleaned up more.


Please contact me if you wish to contribute, I could use some help!
