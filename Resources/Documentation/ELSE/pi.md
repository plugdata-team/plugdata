---
title: pi

description: get the value of Pi

categories:
- object

pdcategory: ELSE, Data Math

arguments:
- description: multiplier
  type: float
  default: 1

inlets:
  1st:
  - type: bang
    description: calculate or output the last calculated value
  2nd:
  - type: float
    description: multiplier

outlets:
  1st:
  - type: float
    description: the value of pi

draft: false
---

[pi] calculates and outputs the value of pi. It receives a multiplier value via the argument or second inlet, which needs to be greater than 0 (otherwise it's considered as 1).

