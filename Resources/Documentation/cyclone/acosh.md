---
title: acosh
description: hyperbolic arc-cosine function
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
    description: input to hyperbolic arc-cosine function, this value is stored and updates the argument
  - type: bang
    description: calculates output according to the stored input
outlets:
  1st:
  - type: float
    description: the hyperbolic arc-cosine of the input

---

[acosh] calculates the hyperbolic arc-cosine of a given number.

