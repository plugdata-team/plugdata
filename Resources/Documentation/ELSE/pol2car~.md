---
title: pol2car~

description: polar to Cartesian conversion

categories:
- object

pdcategory: ELSE, Signal Math, Converters

arguments:

inlets:
  1st:
  - type: float/signal
    description: amplitude of the signal in the polar form
  2nd:
  - type: float/signal
    description: phase (in radians) of the signal in the polar form

outlets:
  1st:
  - type: signal
    description: real part of the signal in the Cartesian form
  2nd:
  - type: signal
    description: imaginary part of the signal in the Cartesian form

draft: false
---

[pol2car~] converts polar coordinates (amplitude / phase) to Cartesian coordinates (real / imaginary).

