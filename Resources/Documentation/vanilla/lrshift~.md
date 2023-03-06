---
title: lrshift~
description: shift signal vector elements left or right
categories:
- object
see_also:
- tabread4~
pdcategory: vanilla, Signal Math, Effects
last_update: '0.31'
inlets:
  1st:
  - type: signal
    description: input signal vector to shift
outlets:
  1st:
  - type: signal
    description: shifted signal vector
arguments:
- type: float
  description: shift amount, positive or negative 
  default: 0
draft: false
---
Acting at whatever vector size the window is running at, lrshift~ shifts samples to the left (toward the beginning sample
