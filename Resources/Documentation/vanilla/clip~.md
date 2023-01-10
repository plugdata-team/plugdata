---
title: clip~
description: restrict a signal between two limits
categories:
- object
pdcategory: Audio Math
last_update: '0.33'
see_also:
- min~
- max~
- clip
- expr~
arguments:
- description: initial lower limit 
  default: 0
  type: float
- description: initial upper limit 
inlets:
  1st:
  - type: signal
    description: signal value to clip
  2nd:
  - type: float
    description: set lower limit
  3rd:
  - type: float
    description: set upper limit
outlets:
  1st:
  - type: signal
    description: the clipped signal

