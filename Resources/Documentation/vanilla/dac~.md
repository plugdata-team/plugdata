---
title: dac~
description: audio output to sound card
categories:
- object
pdcategory: vanilla, Audio I/O
last_update: '0.47'
inlets:
  1st:
  - type: set <list>
    description: resets the channel(s)
  nth:
  - type: signal
    description: signal output to sound card
arguments:
  - type: list
    description: set output channels 
  default: 1 2
draft: false
---
adc~ and dac~ provide real-time audio input and output for Pd, respectively, whether analog or digital. By default they are stereo (with channel numbers 1, 2
