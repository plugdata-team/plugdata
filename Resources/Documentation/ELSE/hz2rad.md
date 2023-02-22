---
title: hz2rad
description: Hz to radians-per-sample conversion

categories:
 - object

pdcategory: ELSE, Data Math, Converters

arguments:
- type: float
  description: initial frequency value
  default: 0

inlets:
  1st:
  - type: float/list
    description: Hz value(s)
  - type: bang
    description: convert or output the last converted value (only float)

outlets:
  1st:
  - type: float/list
    description: converted radians per sample value(s)

methods:
  - type: set
    description: sets next value to be converted via bang (only float)

draft: false
---

[hz2rad] converts a frequency in Hz to "Radians per Sample" - which depends on the patch's sample rate (sr). The conversion formula is;
rad = (hz * 2*pi / sr).

