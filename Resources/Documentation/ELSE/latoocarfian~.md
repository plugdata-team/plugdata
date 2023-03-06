---
title: latoocarfian~

description: Latoocarfian chaotic generator

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
  default: 3
- type: float
  description: sets c
  default: 0.5
- type: float
  description: sets d
  default: 0.5
- type: float
  description: sets initial value of y[n-1]
  default: 0.5
- type: float
  description: sets initial value of x[n-1]
  default: 0.5

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 2 floats set x[n-1] and y[n-1], respectively

outlets:
  1st:
  - type: signal
    description: Latoocarfian chaotic signal

methods:
  - type: coeffs <f, f, f, f>
    description: list sets values of 'a', 'b', 'c' and 'd'

draft: false
---

[latoocarfian~] is a chaotic generator using the difference equations;
x[n] = sin(a * y[n-1]) + d * sin(a * x[n-1]);
y[n] = sin(b * x[n]) + c * sin(b * y[n-1]);

