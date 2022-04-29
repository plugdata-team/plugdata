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
- description: initial lower limit (default 0).
  type: float
- description: initial upper limit (default 0).
  type: float
inlets:
  1st:
  - type: bang
    description: re clip last incomming number between the two limits.
  - type: float
    description: number to clip.
  2nd:
  - type: float
    description: set lower limit.
  3rd:
  - type: float
    description: set upper limit.
outlets:
  1st:
  - type: float
    description: the clipped value.
draft: false
---
Bound a number between two limits.

The clip object passes its signal input to its output,  clipping it to lie between two limits.
