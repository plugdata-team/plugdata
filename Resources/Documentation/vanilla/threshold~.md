---
title: threshold~
description: trigger from audio signal generator
categories:
- object
pdcategory: vanilla, Analysis, Effects, Signal Math
last_update: '0.32'
see_also:
- line
- line~
inlets:
  1st:
  - type: signal
    description: signal to analyze and trigger from
  2nd:
  - type: float
    description: non-0 sets internal state to 'high', 'low' otherwise
outlets:
  1st:
  - type: bang
    description: bang when reaching or exceeding a trigger threshold
  2nd:
  - type: bang
    description: bang when receding below a rest threshold
arguments:
  - type: float
    description: trigger threshold value
  - type: float
    description: trigger debounce time in ms
  - type: float
    description: rest threshold value
  - type: float
    description: rest debounce time in ms
methods:
  - type: set <list>
    description: set different values for the 4 arguments
draft: false
---
threshold~ monitors its input signal and outputs bangs when the signal reaches or exceeds a specified "trigger" value, and also when the signal recedes below a "rest" value - this is also known as a 'schmitt trigger'. You can specify debounce times in milliseconds for the threshold~ to wait after the two event types before triggering again.
