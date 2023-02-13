---
title: resonant2~

description: resonant filter with attack/decay time

categories:
 - object

pdcategory: ELSE, Filters, Effects

arguments:
- type: float
  description: center frequency in Hz
  default: 0
- type: float
  description: attack time in ms
  default: 0
- type: float
  description: decay time in ms
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be filtered or excite the resonator
  2nd:
  - type: float/signal
    description: central frequency in Hz
  3rd:
  - type: float/signal
    description: attack time in ms
  4th:
  - type: float/signal
    description: decay time in ms

outlets:
  1st:
  - type: signal
    description: resonator/filtered signal

  
methods:
- type: clear
    description: clears filter's memory
  
draft: false
---    

[resonant2~] is a resonator just like [resonant~], but you you can specify an attack time besides a decay time.
