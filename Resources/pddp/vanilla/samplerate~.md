---
title: samplerate~
description: get sample rate
categories:
- object
see_also:
- block~
- receive
pdcategory: General Audio Manipulation
last_update: '0.47'
inlets:
  1st:
  - type: bang
    description: output current sample rate.
outlets:
  1st:
  - type: float
    description: sample rate value in Hz.
draft: false
---
When sent a 'bang' message, samplerate~ outputs the current audio sample rate. If called within a subwindow that is up- or down-sampled, the sample rate of signals within that subwindow are reported.

