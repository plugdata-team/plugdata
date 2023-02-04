---
title: lop2~

description: 1st order lowpass filter

categories:
- object

pdcategory: ELSE, Filters

arguments:
- description: cutoff frequency
  type: float
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float
    description: cutoff frequency

outlets:
  1st:
  - type: signal
    description: filtered signal

draft: false
---

[lop2~] is a 1st order lowpass filter (1-pole, 1-zero).

