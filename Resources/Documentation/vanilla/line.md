---
title: line
description: send a series of linearly stepped numbers
categories:
- object
pdcategory: Time
last_update: '0.48'
see_also:
- line~
- vline~
arguments:
- description: time grain in ms 
  default: 20 ms
  type: float
inlets:
  1st:
  - type: float
    description: set target value and start ramp.
  - type: set <float>
    description: set initial ramp value.
  - type: stop
    description: stop the ramp.
  2nd:
  - type: float
    description: set next ramp time (cleared when ramp starts