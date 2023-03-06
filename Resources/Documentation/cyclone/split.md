---
title: split

description: split numbers according to a range

categories:
 - object

pdcategory: cyclone, Data Math

arguments:
- type: float
  description: initial minimum range 
  default: 0
- type: float
  description: initial maximum range
  default: 0

inlets:
  1st:
  - type: float
    description: numbers to be split
  - type: list
    description: <in, min, max>
  2nd:
  - type: float
    description: set minimum range value
  3rd:
  - type: float
    description: set maximum range value

outlets:
  1st:
  - type: float
    description: input number that is in a given range
  2nd:
  - type: float
    description: input number that is outside a given range

draft: true
---

[split] splits numbers in a given range from numbers outside it. If an input is in between a min/max value or equal to them, the value is sent to the left outlet, or to the right outlet otherwise. Unlike MAX, it only deals with floats.
