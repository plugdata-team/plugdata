---
title: asinh
description: hyperbolic arc-sine function
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
    description: input to hyperbolic arc-sine function, this value is stored and updates the argument
  - type: bang
    description: calculates output according to the stored input
outlets:
  1st:
  - type: float
    description: the hyperbolic arc-sine of the input

---

Use the [asinh] object to calculate and output the hyperbolic arc-sine of any given number.

