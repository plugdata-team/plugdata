---
title: henon~
description: Henon map chaotic generator

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: sets frequency in hertz
  default: nyquist
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
    description: frequency in hertz (negative values accepted)
  - type: list
    description: 2 floats set y[n-1] and y[n-2], respectively

outlets:
  1st:
  - type: signal
    description: henon map chaotic signal

methods:
  - type: coeffs <f, f>
    description: floats set 'a' and 'b', respectively

---

[henon~] is a chaotic generator using the difference equation;
y[n] = 1 - a * y[n-1]^2 + b * y[n-2]

