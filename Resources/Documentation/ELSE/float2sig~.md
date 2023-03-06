---
title: float2sig~, f2s~
description: convert float to signal

categories:
 - object

pdcategory: ELSE, Signal Math, Data Math, Converters

arguments:
- type: float
  description: ramp time in ms
  default: 0
- type: float
  description: initial output value
  default: 0

inlets:
  1st:
  - type: float
    description: float value to convert to signal
  2nd:
  - type: float
    description: ramp time
outlets:
  1st:
  - type: signal
    description: converted signal

draft: false
---

[float2sig~] (or [f2s~] for short) converts floats to signals. The conversion is smoothened by a ramp time in ms.

