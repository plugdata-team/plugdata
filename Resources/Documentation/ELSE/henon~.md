---
title: henon~
description: Henon map chaotic generator

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets a
  default: 1.4
- type: float
  description: sets b
  default: 0.3
- type: float
  description: sets initial value of y[n-1]
  default: 0
- type: float
  description: sets initial value of y[n-2]
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 2 floats set y[n-1] and y[n-2], respectively

outlets:
  1st:
  - type: signal
    description: Henon map chaotic signal

methods:
  - type: coeffs <f, f>
    description: floats set 'a' and 'b', respectively

draft: false
---

[henon~] is a chaotic generator using the difference equation;
y[n] = 1 - a * y[n-1]^2 + b * y[n-2]

