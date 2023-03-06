---
title: poltocar

description: polar to cartesian conversion

categories:
 - object

pdcategory: cyclone, Converters

arguments: (none)

inlets:
  1st:
  - type: bang
    description: converts the last received coordinates pair
  - type: float
    description: amplitude from the polar coordinates
  2nd:
  - type: float
    description: phase (in radians) of the signal in the polar form

outlets:
  1st:
  - type: float
    description: real part of the cartesian form
  2nd:
  - type: float
    description: imaginary part of the cartesian form

draft: true
---

[poltocar] converts polar coordinates (amplitude / phase) to cartesian coordinates (real / imaginary).
