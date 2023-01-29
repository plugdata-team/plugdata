---
title: freq.shift~
description: frequency shifter

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: the frequency shift value in hertz
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be shifted in frequency
  2nd:
  - type: float/signal
    description: the requency shift value

outlets:
  1st:
  - type: signal
    description: frequency shifted signal

methods:
  - type: clear
    description: clears filter's memory

---

[freq.shift~] has two outputs, the left outlet is the frequency shifted signal and the right outlet is the signal shifted in the opposite direction.

