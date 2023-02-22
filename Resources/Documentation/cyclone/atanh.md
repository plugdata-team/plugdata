---
title: atanh
description: hyperbolic arc-tangent function
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
    description: input to hyperbolic arc-tangent function, this value is stored and updates the argument
  - type: bang
    description: calculates output according to the stored input
outlets:
  1st:
  - type: float
    description: the hyperbolic arc-tangent of the input

draft: false
---

use the [atanh] object to calculate the hyperbolic arc-tangent of any given number.

