---
title: lcm

description: least common multiple

categories:
- object

pdcategory: ELSE, Data Math

arguments:
- description: right inlet value
  type: float
  default: 1

inlets:
  1st:
  - type: float
    description: set number 1 and output value
  - type: list
    description: sets a list of numbers and output value
  2nd:
  - type: float
    description: set number 2

outlets:
  1st:
  - type: float
    description: lcm of two or more floats

draft: false
---

[lcm] returns the least common multiple of two or more numbers. Floats are truncated to integers. Negative values are accepted, "0" is treated as "1". It has two inlets for setting two floats, but a list input to the left sets 2 or more floats to get their lcm.

