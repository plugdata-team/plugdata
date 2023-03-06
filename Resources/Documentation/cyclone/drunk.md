---
title: drunk
description: random drunk walk
categories:
 - object
pdcategory: cyclone, Random and Noise
arguments:
- type: float
  description: sets maximum value
  default: 128
- type: float
  description: sets maximum stepsize
  default: 2
inlets:
  1st:
  - type: bang
    description: triggers a random output
  - type: float
    description: sets new current value and outputs it
  - type: list
    description: of floats sets <value, maximum, step size> in this order
  2nd:
  - type: float
    description: sets upper bound
  3rd:
  - type: float
    description: sets stepsize
outlets:
  1st:
  - type: float
    description: random number output as result of the drunken walk

methods:
  - type: seed <float>
    description: seeds the internal random number generator
  - type: set <float>
    description: sets the current value (without output)

draft: false
---

[drunk] generates random numbers within a given step range from the current number (generating a "drunk walk"). The random number is from 0 to a given maximum and differs from the previous number by a random value less than the given step size.

