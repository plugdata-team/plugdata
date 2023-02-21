---
title: freq.shift~
description: frequency shifter

categories:
 - object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: the frequency shift value in Hz
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be shifted in frequency
  2nd:
  - type: float/signal
    description: the frequency shift value

outlets:
  1st:
  - type: signal
    description: frequency shifted signal
  2nd:
  - type: signal
    description: signal shifted in the opposite direction

methods:
  - type: clear
    description: clears filter's memory

draft: false
---

[freq.shift~] has two outputs, the left outlet is the frequency shifted signal and the right outlet is the signal shifted in the opposite direction.

