---
title: output~
description: simple stereo output GUI abstraction.
categories:
- object
see_also:
- dac~
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
last_update: '0.52'
inlets:
  1st:
  - type: signal
    description: left signal input.
  - type: level <float> 
    description: sets output level.
  - type: bang
    description: mute/unmute.
  2nd:
  - type: signal
    description: right signal input.
draft: false
---
This is a very simple abstraction that is widely used in Pd's documentation (help files and examples). It is included here just for convenience.

Output~ takes a stereo signal and has GUI controls to set the volume gain and mute it. The number box sets the output gain in dB (from 0 to 100) and whenever you click on it to drag it, the DSP engine is turned on. The mute bang turns the volume off (sets to 0) but restores to the last setting when you click back on it to unmute.

The left inlet also takes a bang message to mute/unmute and a 'level' message to control the level output in dB.