---
title: abs~
description: Calculates absolute value of signal
categories:
- object
pdcategory: Audio Math
last_update: '0.42'
see_also:
- abs
- expr~
inlets:
  1st:
  - type: signal
    description: Input for absolute value
outlets:
  1st:
  - type: signal
    description: Absolute value
arguments:
  - type: float 
    description: Initial value
draft: false
---
The abs~ object passes nonnegative values unchanged, but replaces negative ones with their (positive) inverses.