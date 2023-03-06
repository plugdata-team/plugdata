---
title: rminus, !-, cyclone/!-

description: reversed inlets subtraction

categories:
 - object

pdcategory: cyclone, Data Math

arguments:
- type: float
  description: sets an initial value to subtract from
  default: 0

inlets:
  1st:
  - type: float
    description: the subtrahend (hot inlet)
  - type: bang
    description: performs the subtraction with the numbers currently stored
  2nd:
  - type: float
    description: the minuend (cold inlet)

outlets:
  1st:
  - type: float
    description: the difference of the two numbers

draft: true
---

[rminus] or [!-] is like the [-] object, but the inlets' functions are reversed.
