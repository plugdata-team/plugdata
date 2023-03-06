---
title: hv.comb~

description: comb filter

categories:
- object

pdcategory: heavylib, Filters

arguments:

inlets:
  1st:
  - type: signal
    description: input signal
  2nd:
  - type: feedfwd <float>
    description: feedforward amount (0 to 0.999)
  - type: feedback <float>
    description: feedback amount (0 to 0.999)
  - type: indelay <float>
    description: input delay time (0 to 50ms)
  - type: outdelay <float>
    description: output delay time (0 to 50ms)
  - type: slide <float>
    description: delay update interpolation time (0 to 1000ms)

outlets:
  1st:
  - type: signal
    description: filtered signal

draft: false
---



