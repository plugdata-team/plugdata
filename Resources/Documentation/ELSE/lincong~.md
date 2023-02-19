---
title: lincong~

description: linear congruential chaotic generator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets 'a'
  default: 1.1
- type: float
  description: sets 'c'
  default: 0.13
- type: float
  description: sets 'm'
  default: 1
- type: float
  description: sets initial value of y[n-1]
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 4 floats sets 'a', 'c', 'm' and y[n-1]

outlets:
  1st:
  - type: signal
    description: linear congruential chaotic signal

draft: false
---

[lincong~] is a chaotic generator using the difference equation;
y[n] = (a * y[n-1] + c) % m - the output signal is scaled to a range of -1 to 1 automatically.

