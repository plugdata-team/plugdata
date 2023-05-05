---
title: rad2hz

description: radians-per-sample to Hz conversion

categories:
 - object

pdcategory: ELSE, Data Math, Converters

arguments:
- none

inlets: 
  1st:
  - type: list
    description: radians per sample value(s)

outlets:
  1st:
  - type: list
    description: converted Hz value(s)

draft: false
---

Use [rad2hz] to convert a signal representing a frequency in "Radians per Sample" to Hz. This depends on the patch's sample rate (sr). The conversion formula is;
hz = rad * sr / 2pi
