---
title: metro
description: send a message periodically
categories:
- object
pdcategory: Time
see_also:
- delay
- text sequence
- timer
arguments:
- description: Initial metronome time (default 0)
  default: 0
  type: float
- description: Tempo value (default 1)
  default: 1
  type: float
- description: Time unit (default 'msec')
  type: symbol
inlets:
  1st:
  - type: float/bang
    description: Start/Stop metronome
  2nd:
  - type: float
    description: Set metronome interval
outlets:
  1st:
  - type: bang
    description: Metronome output
