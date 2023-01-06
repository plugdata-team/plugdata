---
title: exp~
description: exponential function
categories:
- object
pdcategory: Audio Math
last_update: '0.47'
see_also:
- exp
- sqrt~
- pow~
- log~
- expr~
inlets:
  1st:
  - type: signal
    description: input value to exp function.
outlets:
  1st:
  - type: signal
    description: output of exp function.
arguments:
  - type: float 
    description: initial base value.
draft: false
---
Exp~ raises the Euler number 'e' (about 2.718), to the power of the input signal.