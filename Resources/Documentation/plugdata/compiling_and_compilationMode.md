# Compiling and Compilation Mode
Not only can you run Plug Data as an audio plugin inside your DAW, Plug Data provides the ability to compile your Plug Data code for multiple platforms, including audio plugins. The platforms currently supported are
- C++ code
	- C++ source code for use in other DSP projects coded in C++
- Electro-Smith Daisy
	- Compilation and flashing of code to the memory of a Daisy microcontroller produced by [Electro-Smith](https://electro-smith.com).
- DPF Audio Plugin
	- An audio plugin using the DISTRHO Plugin Framework to compile a ready to use plugin (binary export) or source code export. Plugins can be effects, instruments, or custom plugins, with MIDI I/O, and export as nearly every popular audio plugin format today: LV2, VST2, VST3, CLAP, and JACK.
- Pd External
	- You can compile your code into a Pd External object, similar to creating a Pd abstraction, except an external object uses less computational resources, because an abstraction requires the program to perform a translation of C++ code for every object in a patch, but when compiled to an external, it translates C++ code once for one single object.

## Compilation Mode
In the main plugdata menu, you will find a toggle box labelled ***Compiled Mode***. <br>
![plugdata Compilation Mode-screenShot](https://github.com/thouldcroft/plugdata/assets/1238556/46c9aeeb-24a1-4ebd-b47c-2187f2b7bedc)<br>
Checking this box may (or may not) produce a good number of error messages in the plugdata Console. Compilation mode checks your patch for compliance with the compilation tools included in plugdata. plugdata uses the [Heavy hvcc compiler](https://wasted-audio.github.io/hvcc/docs/01.introduction.html#what-is-heavy) by [Wasted Audio](https://wasted.audio/). hvcc can only compile plugins using a portion of the objects included with plugdata. These objects themselves are a subset of the Pure Data Vanilla set of objects. ***Compiled Mode*** indicates if there are objects in your patch that cannot be used in a compiled patch by posting a message to the Console, and outlining the object in question. <br> <br>
<br>


>[!WARNING]
>When compiling *DPF Audio Plug-ins*, your plugdata patch cannot contain any "special characters". Special characters include the following: <br>
>		` ~ @ ! $ # ^ * % & ( ) [ ] { } < > + = _ – | / \ ; : ' “ , . ?

>[!WARNING]
>When compiling *DPF Audio Plug-ins* and exporting *plug-in binaries*, as of the current version (0.8.0), the destination path for the binaries cannot contain *any* spaces.
