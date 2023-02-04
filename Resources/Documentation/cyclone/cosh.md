---
title: cosh
description: hyperbolic cosine function
categories:
 - object
pdcategory: cyclone, Data Math
arguments:
- type: float
  description: initially stored input value
  default: 0
inlets:
  1st:
  - type: float
    description: input to hyperbolic cosine function, this value is stored and updates the argument
  - type: bang
    description: calculates output according to the stored input
outlets:
  1st:
  - type: float
    description: the hyperbolic cosine of the input

---

[cosh] calculates the hyperbolic cosine of a given number.

