---
title: pol2car

description: Polar to cartesian conversion

categories:
- object

pdcategory:

arguments:
- description:
  type:
  default:

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
    description: real part of the cartesian form
  2nd:
  - type: float
    description: imaginary part of the cartesian form

draft: false
---

[pol2car] converts polar coordinates (amplitude / phase) to cartesian coordinates (real / imaginary).

