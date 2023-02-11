---
title: setdsp~

description: DSP on/off interface

categories:
- object

pdcategory: ELSE, UI, Audio I/O 

arguments:
- type: float
  description: non-0 - on, 0 - off 
  default: current DSP state

inlets:
  1st:
  - type: float
    description: non-0 - on, 0 - off
  - type: bang
    description: changes status

outlets:
  1st:
  - type: float
    description: dsp status - 1 (on), 0 (off)

draft: false
---

[setdsp~] is a convenient abstraction to display and set Pd's audio engine (aka 'DSP') state. You can just click on it to change it to On/Off via its toggle interface. This object is commonly found in the documentation of the ELSE library audio objects, at the top right corner.
By default, the [setdsp~] object will check the current DSP state and load accordingly (on or off). But you can give it an argument to set it on/off.
