---
title: tremolo~

description: Amplitude modulation

categories:
- object

pdcategory:

arguments:
  1st:
  - type: float
    description: temolo frequency
    default: 0
  2nd:
  - type: float
    description: tremolo depth
    default: 1

inlets:
 1st:
  - type: signal
    description: input signal to be processed
  2nd:
  - type: float
    description: temolo frequency
  3rd:
  - type: float
    description: tremolo depth (0-1)

outlets:
  1st:
  - type: signal
    description: tremoloed signal

draft: false
---

The [tremolo~] abstraction performs amplitude modulation It takes a signal input, a tremolo frequency and a tremolo depth. Classic amplitude modulation is achieved with a depth of 1!