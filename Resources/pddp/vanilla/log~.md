---
title: log~
description: logarithms for signals.
categories:
- object
pdcategory: Audio Math
last_update: '0.42'
see_also:
- pow
- sqrt~
- exp~
- log
- expr~
- +~
inlets:
  1st:
  - type: signal
    description: input value to log function.
  2nd:
  - type: signal
    description: set base value of the log function.
outlets:
  1st:
  - type: signal
    description: output of log function.
arguments:
  - type: float 
    description: initial base value.
draft: false
---
computes the logarithm of the left inlet, to the base 'e' (about 2.718), or to another base specified by the inlet or a creation argument.