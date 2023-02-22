---
title: line
description: send a series of linearly stepped numbers
categories:
- object
pdcategory: vanilla, Triggers and Clocks
last_update: '0.48'
see_also:
- line~
- vline~
arguments:
- description: initial ramp value
  default: 0
  type: float
- description: time grain in ms
  default: 20 ms
  type: float
inlets:
  1st:
  - type: float
    description: set target value and start ramp
  2nd:
  - type: float
    description: set next ramp time (cleared when ramp starts)
  3rd:
  - type: float
    description: sets time grain in ms
outlets:
  1st:
  - type: float
    description: ramp values

methods:
  - type: set <float>
    description: set initial ramp value
  - type: stop
    description: stop the ramp
draft: false
---
