---
title: crusher~

description: bitcrush/decimator

categories:
- object

pdcategory: ELSE, Effects

arguments:
  - description: bit reduction
    type: float
  - description: decimation
    type: float

inlets:
  1st:
  - type: signal
    description: signal to be crushed (resampled and quantized)
  2nd: 
  - type: float/signal
    description: bit reduction from 0-1
  3rd:
  - type: float/signal
    description: decimation (sample rate reduction) from 0-1

outlets:
  1st:
  - type: signal
    description: resampled and quantized signal

draft: false
---

[crusher~] is a bit-crusher/reducer and decimator abstraction based on the objects [quantizer~] and [downsample~], where an input signal is resampled and quantized.
