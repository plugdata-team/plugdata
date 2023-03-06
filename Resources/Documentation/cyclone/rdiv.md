---
title: rdiv, !/, cyclone/!/

description: reversed inlets division

categories:
 - object

pdcategory: cyclone, Data Math

arguments:
- type: float
  description: sets an initial value for the dividend
  default: 0

inlets:
  1st:
  - type: float
    description: the divisor (hot inlet)
  - type: bang
    description: performs the division with the numbers currently stored
  2nd:
  - type: float
    description: the dividend

outlets:
  1st:
  - type: float
    description: the division of the two numbers

draft: true
---

[rdiv] or [!/] divides a number by the incoming value on the left inlet. Functions like the [/] object, but the inlets' functions are reversed.
