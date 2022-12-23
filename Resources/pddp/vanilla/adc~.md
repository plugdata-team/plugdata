---
title: adc~
description: Audio input from sound card
categories:
- object
pdcategory: General Audio Manipulation
last_update: '0.47'
inlets:
  1st:
  - type: set <list>
    description: resets the channel(s).
outlets:
  nth:
  - type: signal
    description: Signal input from sound card.
arguments:
  - type: list
    description: Set input channels
    default: 1 2
draft: false
---
adc~ and dac~ provide real-time audio input and output for Pd, respectively, whether analog or digital. By default they are stereo (with channel numbers 1, 2
