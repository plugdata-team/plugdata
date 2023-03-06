---
title: scale

description: map values to an output range

categories:
 - object

pdcategory: cyclone, Data Math

arguments:
- type: float
  description: input low
  default: 0
- type: float
  description: input high
  default: 127
- type: float
  description: output low
  default: 0
- type: float
  description: output high
  default: 1
- type: float
  description: exponentiation factor
  default: 1 (linear)

inlets:
  1st:
  - type: float/list
    description: value(s) to perform the scaling function on
  - type: bang
    description: performs the scaling function on last received input
  2nd:
  - type: float
    description: sets the low end of the input range
  3rd:
  - type: float
    description: sets the high end of the input range
  4th:
  - type: float
    description: sets the low end of the output range
  5th:
  - type: float
    description: sets the high end of the output range
  6th:
  - type: float
    description: sets the exponentiation factor

outlets:
  1st:
  - type: signal
    description: the scaled values according to output range

flags:
  - name: @classic <int>
    description: 0 — "modern", 1 — "classic" mode
    default: 0

methods:
  - type: classic <float>
    description: sets to classic (1) or modern (0) mode

draft: false
---

[scale] maps an input range to an output range. Values larger or smaller than the input range will not be clipped to the output range. The mapping can be inverted and/or exponential.
