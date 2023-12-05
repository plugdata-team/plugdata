# Compiling and Compilation Mode
Not only can you run Plug Data as an audio plugin inside your DAW, Plug Data provides the ability to compile your Plug Data code for multiple platforms, including audio plugins. The platforms currently supported are
- C++ code
	- C++ source code for use in other DSP projects coded in C++
- Electro-Smith Daisy
	- Compilation and flashing of code to the memory of a Daisy microcontroller produced by [Electro-Smith](https://electro-smith.com).
- DPF Audio Plugin
	- An audio plugin using the DISTRHO Plugin Framework to compile a ready to use plugin (binary export) or source code export. Plugins can be effects, instruments, or custom plugins, with MIDI I/O, and export as nearly every popular audio plugin format today: LV2, VST2, VST3, CLAP, and JACK.
- Pd External
	- Compile your plugdata into an external object that can be used in PureData or plugdata like a built-in object.

## Compilation Mode
In the main plugdata menu, you will find a toggle box labelled ***Compiled Mode***. <br>
![plugdata Compilation Mode-screenShot](https://github.com/thouldcroft/plugdata/assets/1238556/46c9aeeb-24a1-4ebd-b47c-2187f2b7bedc)<br>
Checking this box may (or may not) produce a good number of error messages in the plugdata Console. Compilation mode checks your patch for compliance with the compilation tools included in plugdata. plugdata uses the [Heavy hvcc compiler](https://wasted-audio.github.io/hvcc/docs/01.introduction.html#what-is-heavy) by [Wasted Audio](https://wasted.audio/). hvcc can only compile plugins using a portion of the objects included with plugdata. These objects themselves are a subset of the Pure Data Vanilla set of objects. ***Compiled Mode*** indicates if there are objects in your patch that cannot be used in a compiled patch by posting a message to the Console, and outlining the object in question.<br>
<br> 
<img width="145" alt="PlugData Compilation Mode - unsupported object" src="https://github.com/thouldcroft/plugdata/assets/1238556/08bd1135-3fb2-4b24-b5e2-0be64f9cc864"><br>
<sub>Object error indication when using an unsupported object in Compilation Mode</sub><br>
<br>
<img width="251" alt="PlugData Compilation Mode - unsupported object console warning" src="https://github.com/thouldcroft/plugdata/assets/1238556/1f5c4a26-0cd1-4a68-9e66-5666b4ff5b9b"><br>
<sub>Console error warning when using an unsupported object in Compilation Mode</sub><br>
<br>

## Compiling in plugdata
When you select "Compile..." a window with different compilation options along the left, the primary window displaying the common options for all compilation modes and then specific qualities for the compilation mode you currently have selected.

### General<br>
![Plugdata Compilation Mode - General](https://github.com/thouldcroft/plugdata/assets/1238556/2a2c126f-aeff-4d90-a19f-118cafc46db1)

This section is found in every compilation mode, with the following fields:
- Patch to export
	- If a patch is currently open, it will read "Currently opened patch" otherwise it will read "Choose a patch to export...". Select the patch you wish to export here.
 - Project Name (Optional)
 	- The name of the project (potentially different than the patch name) that is used in some compilation scenarios in their information registry. This will autofill to your patch's name, or the last name you typed in this field.
  - Project Copyright (optional)
  	- Similar to the Project Name, the Project Copyright is placed in the information registry for the compilation mode 	

## C++ Code<br>
<img width="634" alt="Plugdata Compiler C++ Code" src="https://github.com/thouldcroft/plugdata/assets/1238556/25182673-eef3-40fb-913e-cecdb8311625"><br>
In the C++ Code mode, your plugdata patch isn't actually compiled, instead it is [transpiled/transcompiled](https://en.wikipedia.org/wiki/Source-to-source_compiler) from one coding language (Pd) to another (C++). The plugdata patch is transpiled for no specific platform, and the raw code can then be used for any project that accepts C++ code.

## Electro-Smith Daisy<br>
<img width="635" alt="Plugdata Compiler Daisy Export" src="https://github.com/thouldcroft/plugdata/assets/1238556/d206902c-2115-4727-be2a-27b23b2ab821"><br>
 ***TBD***

## DPF Audio Plugin<br>
<img width="636" alt="Plugdata Compiler Plugin Export" src="https://github.com/thouldcroft/plugdata/assets/1238556/63e21b8c-8ed0-4131-8206-7a7be361b58f"><br>While plugdata itself can run as a VST3, LV2, CLAP or AU plugin, you can also use the plugdata standalone app to produce standalone plugins in the VST2&3, LV2, CLAP, and JACK formats. HVCC uses the DISTRHO Plugin Framework to generate these plugins.<br>
### DPF<br>
This section defines specifics for the compiled plugin.
- Export type
	- **Source code** compiles the plugdata patch into C++ source code that can be externally compiled into an audio plugin 	
	- **Binary** produces an executable plugin that can be loaded into any plugin host.
 - Plugin type
 	- The plugin type is used by some hosts to categorize plugins, the type also automatically defines the MIDI input/output parameters based on the type	 
 	- **Effect** is a plug in that accepts input audio, processes the audio, and outputs audio. It doesn't accept MIDI nor does it output MIDI.
	- **Instrument** is a plugin that accepts MIDI input to control the software instrument contained in the plugin, and outputs audio. Will not accept audio nor output MIDI.
	- **Custom** allows you to break with the effect or instrument conventions. This would be the type to choose if you wanted to create a plugin that modifies MIDI, for instance, as you can set the plugin to accept MIDI input but set it to also output MIDI data (also known as a MIDI effect).
### Plugin formats  
<img width="425" alt="Plugdata Compiler Plugin Formats" src="https://github.com/thouldcroft/plugdata/assets/1238556/b3643f41-81e0-431e-a814-14970ee9fb86"><br>
Here you define the formats you wish to output your binary as, or the formats the source code will accomodate. You can select as many or as few of formats as you like. JACK format is deselected by default.


>[!WARNING]
>When compiling *DPF Audio Plug-ins*, your plugdata patch cannot contain any "special characters". Special characters include the following: <br>
>		` ~ @ ! $ # ^ * % & ( ) [ ] { } < > + = _ – | / \ ; : ' “ , . ?

>[!WARNING]
>When compiling *DPF Audio Plug-ins* and exporting *plug-in binaries*, as of writing, the destination path for the binaries cannot contain *any* spaces.

## Pd External<br>
<img width="638" alt="Plugdata Compiler External" src="https://github.com/thouldcroft/plugdata/assets/1238556/47cb9cbd-4963-43e6-b1c8-7311baad9095"><br>
One of the few detractions from a visual programming language like plugdata is that for every object in your code, C code is working behind the scenes, with a snippet of C code for every object. plugdata allows you to write abstractions and subpatches that look and operate like built in objects, but abstractions and subpatches can't run as efficiently as an object that is composed of C code. plugdata helps you bridge this gap by providing the functionality to compile your plugdata abstraction into a Pd external, which can be used in plugdata or Pd. 

### Pd
- **Export Type**
	- Export your compiled external as a Binary (an object ready to be used in Pd) or as external object source code for further modification and external compilation
- **Copy to externals path**
	- This will create a copy of the external object in the plugdata/Externals folder  





