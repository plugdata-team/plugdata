---
title: abs~
description: calculates absolute value of signal
categories:
- object
pdcategory: vanilla, Signal Math
last_update: '0.42'
see_also:
- abs
- expr~
inlets:
  1st:
  - type: signal
    description: input for absolute value
outlets:
  1st:
  - type: signal
    description: absolute value
arguments:
  - type: float 
    description: initial value
draft: false
---
The abs~ object passes nonnegative values unchanged, but replaces negative ones with their (positive) inverses.
