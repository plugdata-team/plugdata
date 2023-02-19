---
title: timer
description: measure time intervals
categories:
- object
pdcategory: vanilla, Triggers and Clocks
last_update: '0.47'
see_also:
- cputime
- realtime
- delay
- metro
- text sequence
arguments:
- description: tempo value 
  default: 1
  type: float
- description: time unit 
  default: 'msec'
  type: symbol
inlets:
  1st:
  - type: bang
    description: reset (set elapsed time to zero)
  2nd:
  - type: bang
    description: output elapsed time
outlets:
  1st:
  - type: float
    description: elapsed time in msec
methods:
  - type: tempo <float, symbol>
    description: set tempo value (float) and time unit (symbol)
draft: false
---

