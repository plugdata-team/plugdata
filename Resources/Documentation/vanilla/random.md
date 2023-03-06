---
title: random
description: pseudo random integers
categories:
- object
pdcategory: vanilla, Random and Noise
last_update: '0.33'
see_also:
- expr
arguments:
- description: initial range value 
  default: 1
  type: float
inlets:
  1st:
  - type: bang
    description: generate a random integer number
  2nd:
  - type: float
    description: set the range
outlets:
  1st:
  - type: float
    description: the generated random number
methods:
  - type: seed <float>
    description: set a seed value for repeatable random numbers
draft: false
---
Random outputs pseudo random integers from 0 to N-1 where N is the creation argument. You can specify a seed if you wish. Seeds are kept locally so that if two Randoms are seeded the same they will have the same output (or indeed you can seed the same one twice to repeat the output.
