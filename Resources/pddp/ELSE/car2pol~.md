---
title: car2pol~

description:

categories:
- object

pdcategory: Math (Conversion)

arguments: none

inlets:
  1st:
  - type: float/signal
    description: real part from the cartesian coordinates
  2nd:
  - type: float/signal
    description: imaginary part form the cartesian coordinates

outlets:
  1st:
  - type: signal
    description: amplitude of the polar form
  2nd:
  - type: signal 
    description: phase in radians (-pi to pi) of the polar form

draft: false
---

[car2pol~] converts cartesian coordinates (real / imaginary) to polar coordinates (amplitude / phase).
