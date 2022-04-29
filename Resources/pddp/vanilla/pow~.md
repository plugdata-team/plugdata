---
title: pow~
description: power function for signals
categories:
- object
pdcategory: Audio Math
last_update: '0.42'
see_also:
- pow
- sqrt~
- exp~
- log~
- expr~
- +~
inlets:
  1st:
  - type: signal
    description: input value to power function.
  2nd:
  - type: signal
    description: set numeric power to raise to.
outlets:
  1st:
  - type: signal
    description: output of power function.
arguments:
  - type: float 
    description: initial numeric power.
draft: false
---
pow~ raises a signal to a numeric power (given by another signal or argument/float). The inputs may be positive, zero, or negative.

WARNING: it's easy to generate "infinity" by accident, and if you do, the DSP chain may dramatically slow down if you're using an i386 or ia64 processor. Out-of-range floating point values are thousands of times slower to compute with than in-range ones. There' a protection against NaNs (they become 0).