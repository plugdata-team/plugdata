---
title: adc~
description: audio input from sound card
categories:
- object
pdcategory: General Audio Manipulation
last_update: '0.47'
inlets:
  1st:
  - type: set <list>
    description: resets the channel(s).
outlets:
  'n':
  - type: signal
    description: signal input from sound card.
arguments:
  - type: list
    description: set input channels (default 1 2).
draft: false
---
adc~ and dac~ provide real-time audio input and output for Pd, respectively, whether analog or digital. By default they are stereo (with channel numbers 1, 2) but you can specify different channel numbers via arguments.

The actual number of channels Pd inputs and outputs is set on Pd's command line or in the "audio settings" dialog. You can open patches that want to use more channels, and channel numbers out of range will be dropped in dac~ or output zero in adc~.

If more than one dac~ outputs to the same channel, the signals are added. More than one adc~ can output the same input channel.