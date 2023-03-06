---
title: gcd
description: greatest common divisor

categories:
 - object

pdcategory: ELSE, Data Math

arguments:
- type: float
  description: set right inlet value
  default: 0

inlets:
  1st:
  - type: float
    description: float input value
  - type: list
    description: list of input values
  2nd:
  - type: float
    description: float input value

outlets:
  1st:
  - type: float
    description: greatest common divisor

draft: false
---

[gcd] returns the greatest common divisor of two or more numbers. Floats are truncated to integers. Negative values are accepted, "0" is treated as "1". It has two inlets for setting two floats, but a list input to the left sets 2 or more floats to get their gcd.

