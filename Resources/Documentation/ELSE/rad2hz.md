---
title: rad2hz

description: radians-per-sample to Hz conversion

categories:
 - object

pdcategory: ELSE, Data Math, Converters

arguments:
- type: float
  description: initial radians per sample value
  default: 0

inlets: 
  1st:
  - type: float/list
    description: radians per sample value(s)
  - type: bang
    description: convert or output the last converted value (only float)

outlets:
  1st:
  - type: float/list
    description: converted Hz value(s)

methods:
  - type: set <float>
    description: sets next value to be converted via bang (only float)

draft: false
---

Use [rad2hz] to convert a signal representing a frequency in "Radians per Sample" to Hz. This depends on the patch's sample rate (sr). The conversion formula is;
hz = rad * sr / 2pi
