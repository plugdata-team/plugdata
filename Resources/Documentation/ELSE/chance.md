---
title: chance

description: weighted random

categories:
 - object

pdcategory: ELSE, Triggers and Clocks, Random and Noise

arguments:
- type: list
  description: list of probabilities
  default: 50 50

inlets:
  1st:
  - type: bang
    description: a bang to be passed or not
  - type: list
    description: updates arguments if more than 1 is given
  2nd:
  - type: float
    description: sets chance number if only one argument

outlets:
  nth:
  - type: bang
    description: bangs according to chance

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

When [chance] receives a bang, it outputs to an outlet according to its chance (probability weight). The total number is the sum of all arguments. It also has an index mode where it outputs the outlet index number instead.
