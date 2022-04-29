---
title: abs~
description: absolute value for signals
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
    description: signal input.
outlets:
  1st:
  - type: signal
    description: signal with absolute values.
arguments:
  - type: float 
    description: initial base value.
draft: false
---
The abs~ object passes nonnegative values unchanged, but replaces negative ones with their (positive) inverses.