---
title: car2pol~

description: signal Cartesian to polar conversion

categories:
- object

pdcategory: ELSE, Converters, Signal Math

arguments:
inlets:
  1st:
  - type: float/signal
    description: real part from the Cartesian coordinates
  2nd:
  - type: float/signal
    description: imaginary part form the Cartesian coordinates

outlets:
  1st:
  - type: signal
    description: amplitude of the polar form
  2nd:
  - type: signal 
    description: phase in radians (-pi to pi) of the polar form

draft: false
---

[car2pol~] converts Cartesian coordinates (real / imaginary) to polar coordinates (amplitude / phase).
