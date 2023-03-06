---
title: car2pol

description: Cartesian to polar conversion

categories:
- object

pdcategory: ELSE, Converters, Data Math

arguments:
inlets:
  1st:
  - type: bang
    description: converts the last received coordinates pair
  - type: float
    description: real part from the Cartesian coordinates
  2nd:
  - type: float
    description: imaginary part from the Cartesian coordinates

outlets:
  1st:
  - type: float
    description: amplitude of the polar form
  2nd:
  - type: float
    description: phase in radians (-pi to pi) of the polar form

draft: false
---

[car2pol] converts Cartesian coordinates (real / imaginary) to polar coordinates (amplitude / phase).
