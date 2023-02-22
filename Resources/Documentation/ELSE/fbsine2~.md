---
title: fbsine2~
description: feedback sine with chaotic phase indexing

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets 'im'
  default: 1
- type: float
  description: sets 'fb'
  default: 0.1
- type: float
  description: sets 'a'
  default: 1.1
- type: float
  description: sets 'c'
  default: 0.5
- type: float
  description: sets initial value of x[n]
  default: 0.1
- type: float
  description: sets initial phase value of y[n]
  default: 0.1

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 2 floats set x[n-1] and y[n-1] respectively

outlets:
  1st:
  - type: signal
    description: feedback sine chaotic signal

methods:
  - type: coeffs <f, f, f, f>
    description: sets values of 'im', 'fb', 'a' and 'c', respectively
  - type: clear
    description: clears values of x[n-1] and y[n-1]

draft: false
---

[fbsine2~] is a non-interpolating sound generator based on the difference equations below (for im = 1, fb = 0 and a = 1, a sine wave results);
x(n+1) = sin(im * y(n) + fb * x(n));
y(n+1) = (a * y(n) + c) % 2pi

