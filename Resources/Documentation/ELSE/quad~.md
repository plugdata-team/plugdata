---
title: quad~
description: general quadratic map chaotic generator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets a
  default: 1
- type: float
  description: sets b
  default: 1
- type: float
  description: sets c
  default: -0.75
- type: float
  description: sets initial value of y[n-1]
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 4 floats sets 'a', 'b', 'c', and y[n-1]

outlets:
  1st:
  - type: signal
    description: general quadratic map chaotic signal

draft: false
---

[quad~] generates a chaotic signal from the difference equation;
y[n] = a * y[n-1]^2 + b * y[n-1] + c

