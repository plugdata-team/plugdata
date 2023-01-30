---
title: brickwall~

description: brickwall filter

categories:
- object

pdcategory: ELSE, Filters

arguments:
- description: cutoff in Hz
  type:
  default: 0.75 * Nyquist

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float
    description: cutoff frequency in Hz

outlets:
  1st:
  - type: signal 
    description: the filtered signal

draft: false
---

[brickwall~] a 10th order butterworth lowpass filter, which is filter with a steep cutoff slope (60 dB by octave). By default, its cutoff frequency is 0.75 * Nyquist, but you can change it with the 1st argument or the second inlet.
