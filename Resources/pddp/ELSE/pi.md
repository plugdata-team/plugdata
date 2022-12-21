---
title: pi

description: Get the value of Pi

categories:
- object

pdcategory:

arguments:
- description: multiplier
  type: float
  default: 1

inlets:
  1st:
  - type: bang
    description: calculate or output the last calculated value

outlets:
  1st:
  - type: float
    description: the value of pi

draft: false
---

[pi] calculates and outputs the value of pi. It receives a multiplier value via the argument or second inlet, which needs to be greater than 0 (otherwise it's considered as 1).

