---
title: clip
description: force a number into a range
categories:
- object
pdcategory: Math
last_update: '0.47'
see_also:
- clip~
- expr
- max
- min
arguments:
- description: initial lower limit 
  default: 0
  type: float
- description: initial upper limit 
inlets:
  1st:
  - type: float
    description: number to clip
  - type: bang
    description: re-clip last incoming number between the two limits
  2nd:
  - type: float
    description: set lower limit
  3rd:
  - type: float
    description: set upper limit
outlets:
  1st:
  - type: float
    description: the clipped value

