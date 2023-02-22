---
title: metro
description: send a message periodically
categories:
- object
pdcategory: vanilla, Triggers and Clocks
see_also:
- delay
- text sequence
- timer
arguments:
- description: initial metronome time
  default: 1
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
    description: start the metronome
  - type: float
    description: non-0 starts and zero stops the metronome
  2nd:
  - type: float
    description: set metronome time for the next tempo
outlets:
  1st:
  - type: bang
    description: bang at periodic time

methods:
  - type: stop
    description: stop the metronome
  - type: tempo <float, symbol>
    description: set tempo value and time unit
draft: false
---
