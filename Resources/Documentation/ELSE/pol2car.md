---
title: pol2car

description: polar to Cartesian conversion

categories:
- object

pdcategory: ELSE, Data Math, Converters

arguments:

inlets:
  1st:
  - type: bang
    description: converts the last received coordinates pair
  - type: float
    description: amplitude from the polar coordinates
  2nd:
  - type: float
    description: phase (in radians) from the polar coordinates

outlets:
  1st:
  - type: float
    description: real part of the Cartesian form
  2nd:
  - type: float
    description: imaginary part of the Cartesian form

draft: false
---

[pol2car] converts polar coordinates (amplitude / phase) to Cartesian coordinates (real / imaginary).

