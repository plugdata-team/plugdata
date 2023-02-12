---
title: lfnoise

description: control low frequency noise

categories:
- object

pdcategory: ELSE, Random and Noise, Envelopes and LFOs

arguments:
- description: frequency in Hz
  type: float
  default: 0
- description: interpolation off (0) or on (1)
  type: float
  default: 0

inlets:
  1st:
  - type: float
    description: frequency in Hz up to 100 (negative values accepted)
  2nd:
  - type: bang
    description: reset to a new random value

outlets:
  1st:
  - type: float
    description: low frequency noise output in the range from 0 - 127

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)

methods:
  - type: interp <float>
    description: non-0 sets to linear interpolation
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[lfnoise] is a control rate Low Frequency Noise that outputs random values. It doesn't need the DSP on to function. Give it a seed value if you want a reproducible output.

