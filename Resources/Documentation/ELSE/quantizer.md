---
title: quantizer
description: quantize to steps

categories:
 - object

pdcategory: ELSE, Data Math

arguments:
- type: float
  description: step value
  default: 0, no approximation
- type: float
  description: mode: 0 (round), 1 (int), 2 (floor), or 3 (ceil)
  default: 0

inlets:
  1st:
  - type: float
    description: a float to be quantized
  - type: list
    description: a list of floats to be quantized
  2nd:
  - type: float
    description: step value to quantize to

outlets:
  1st:
  - type: float
    description: quantized float

methods:
  - type: mode <float>
    description: sets approximation mode <0, 1, 2, or 3>

draft: false
---

[quantizer] approximates a value to step values defined by the argument. There are 4 approximation modes: round (default), int, floor & ceil.

