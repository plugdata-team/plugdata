---
title: rad2hz

description: radians_per_sample/Hertz conversion

categories:
 - object

pdcategory: ELSE, Data Math

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
    description: converted hez value(s)

methods:
  - type: set <float>
    description: sets next value to be converted via bang (only float)

draft: false
---

Use [rad2hz] to convert a signal representing a frequency in "Radians per Sample" to Hertz. This depends on the patch's sample rate (sr). The conversion formula is;
hz = rad * sr / 2pi
