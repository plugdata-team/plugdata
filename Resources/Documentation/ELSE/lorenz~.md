---
title: lorenz~

description: Lorenz chaotic generator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets 's'
  default: 10
- type: float
  description: sets 'r'
  default: 28
- type: float
  description: sets 'b'
  default: 2.6667
- type: float
  description: sets 'h'
  default: 0.05
- type: float
  description: sets initial value of x[n-1]
  default: 0.1
- type: float
  description: sets initial value of y[n-1]
  default: 0
- type: float
  description: sets initial value of z[n-1]
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 3 floats set x[n-1], y[n-1] and z[n-1] respectively

outlets:
  1st:
  - type: signal
    description: Lorenz chaotic signal

methods:
  - type: coeffs <f,f,f,f>
    description: list sets values of 's', 'r', 'b' and 'h'
  - type: clear
    description: clears values of z, y and z

draft: false
---

The [lorenz~] is a strange attractor discovered by Edward N. Lorenz while studying mathematical models of the atmosphere.

