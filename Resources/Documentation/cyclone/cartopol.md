---
title: cartopol
description: cartesian to polar conversion
categories:
 - object
pdcategory: cyclone, Converters
arguments:
inlets:
  1st:
  - type: bang
    description: converts the last received coordinates pair
  - type: float
    description: real part from the cartesian coordinates
  2nd:
  - type: float
    description: imaginary part from the cartesian coordinates
outlets:
  1st:
  - type: float
    description: amplitude of the polar form
  2nd:
  - type: float
    description: phase in radians (-pi to pi) of the polar form

---

[cartopol~] converts cartesian coordinates (real / imaginary) to polar coordinates (amplitude / phase).

