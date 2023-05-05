---
title: hz2rad
description: Hz to radians-per-sample conversion

categories:
 - object

pdcategory: ELSE, Data Math, Converters

arguments:
- none

inlets:
  1st:
  - type: list
    description: Hz value(s)

outlets:
  1st:
  - type: list
    description: converted radians per sample value(s)

draft: false
---

[hz2rad] converts a frequency in Hz to "Radians per Sample" - which depends on the patch's sample rate (sr). The conversion formula is;
rad = (hz * 2*pi / sr).

