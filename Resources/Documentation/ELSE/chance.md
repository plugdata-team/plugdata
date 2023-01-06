---
title: chance

description: Weighted random

categories:
 - object

pdcategory: Control (Triggers)

arguments:
- type: list
  description: list of probabilities
  default: 50 50

inlets:
  1st:
  - type: bang
    description: a bang to be passed or not
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  - type: list
    description: updates arguments if more than 1 is given
  2nd:
  - type: float
    description: sets chance number if only one argument

outlets:
  1st:
  - type: bang
    description: random bangs
  2nd:
  - type: float
    description: index number if in index mode

draft: false
---

When [chance] receives a bang, it outputs to an outlet according to its chance (probability weight). The total number is the sum of all arguments. It also has an index mode where it outputs the outlet index number instead.