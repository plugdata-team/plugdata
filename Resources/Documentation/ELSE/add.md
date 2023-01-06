---
title: add

description: Number accumulator

categories:
- object

pdcategory: Math

arguments:
- description: starting sum
  type: float
  default: 0

inlets:
  1st:
  - type: float
    description: value to accumulate
  - type: bang
    description: resets sum to starting point
  - type: set <float>
    description: sets starting sum

outlets:
  1st:
  - type: float
    description: the accumulated value

draft: false
---

[add] starts accumulating input values to a starting sum value (0 by default). The starting sum can be set as an argument or via the 'set' message. You can reset the sum to the starting sum value with a bang to the right inlet.
