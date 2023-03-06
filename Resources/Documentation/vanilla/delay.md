---
title: delay, del
description: send a message after a time delay
categories:
- object
pdcategory: vanilla, Triggers and Clocks, Data Management, Effects
last_update: '0.45'
see_also:
- metro
- pipe
- text sequence
- timer
arguments:
- description: initial delay time
  default: 0
  type: float
- description: tempo value 
  default: 1
  type: float
- description: time unit
  default: msec
  type: symbol
inlets:
  1st:
  - type: bang
    description: start the delay
  - type: float
    description: set delay time and start the delay
  2nd:
  - type: float
    description: set delay time for the next tempo
outlets:
  1st:
  - type: bang
    description: bang at a delayed time

methods:
  - type: stop
    description: stop the delay
  - type: tempo <float, symbol>
    description: set tempo value (float) and time unit symbol
draft: false
---
