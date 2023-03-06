---
title: gbman~
description: gingerbread man chaotic generator

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets initial feedback value of y[n-1]
  default: 1.2
- type: float
  description: initial feedback value of y[n-2]
  default: 2.1

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 2 floats set y[n-1] and y[n-2], respectively

outlets:
  1st:
  - type: signal
    description: gingerbread man map chaotic signal

draft: false
---

[gbman~] is a gingerbread man map chaotic generator, the output is from the difference equation => y[n] = 1 + abs(y[n-1]) - y[n-2].
