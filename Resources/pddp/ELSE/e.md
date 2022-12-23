---
title: e

description: Get the value of 'e'

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
    description: the value of e times the multiplier

draft: false
---

[e] calculates and outputs the value of 'e'. It receives a multiplier value via the argument or second inlet, which needs to be greater than 0 (otherwise it's considered as 1).

