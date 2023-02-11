---
title: cos~
description: cosine waveshaper
categories:
- object
see_also:
- osc~
- cos
- tabread4~
- expr~
pdcategory: vanilla, Effects, Signal Math
last_update: '0.41'
inlets:
  1st:
  - type: signal
    description: input from 0-1 (wraps into 0-1 if outside this range)
outlets:
  1st:
  - type: signal
    description: phase ramp (in the range of 0 to 1)
arguments:
  - type: float
    description: sets input value
draft: false
---
The cos~ object outputs the cosine of two pi times its signal input. So -1, 0, 1 and 2 give 1 out, 0.5 gives -1, and so on.
