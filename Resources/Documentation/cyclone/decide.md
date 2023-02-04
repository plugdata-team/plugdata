---
title: decide
description: randomly output 0 or 1
categories:
 - object
pdcategory: cyclone, Random and Noise
arguments:
- type: float
  description: sets the seed
  default: a random internal one
inlets:
  1st:
  - type: bang
    description: randomly choose between 0 or 1
  - type: float
    description: same as bang
  2nd:
  - type: float
    description: sets the seed for the random number generator. 0 will use a random seed, any other integer float is the seed
outlets:
  1st:
  - type: float
    description: 0 or 1, chosen randomly

---

[decide] alternates randomly from 1 to 0 (the sequence depends on the seed value).

