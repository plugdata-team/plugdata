---
title: rampnoise

description: control ramp noise

categories:
- object

pdcategory: ELSE, Random and Noise

arguments:
- type: float
  description: frequency in Hz
  default: 0

inlets:
  1st:
  - type: float
    description: frequency in Hz (max 100)

outlets:
  1st:
  - type: float
    description: ramp noise's waveform in the range from 0 - 127

flags:
  - name: -seed <float>
    description: sets seed

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[rampnoise] is a control rate ramp noise. It doesn't need the DSP on to function. Give it a seed value if you want a reproducible output.
