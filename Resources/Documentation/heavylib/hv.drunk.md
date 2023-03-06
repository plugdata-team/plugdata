---
title: hv.drunk

description: random walk

categories:
- object

pdcategory: heavylib, Random and Noise

arguments:
- type: float
  description: maximum bound
  default: 100
- type: float
  description: maximum step size
  default: 1

inlets:
  1st:
  - type: bang
    description: generate and output next random number
  2nd:
  - type: float
    description: set maximum bound
  3rd:
  - type: float
    description: set maximum step size

outlets:
  1st:
  - type: float
    description: random walk output

methods:
  - type: seed <float>
    description: set seed

draft: false
---
A random walk (drunk) is a special case of a Markov chain, in which the states are integers and the transitions add or subtract a small amount (or zero) from the previous state to get a new one. Here the "f" holds the state. When it gets a bang, the previous state is added to a random number (from 0 to $bound) multiplied by a random sign (-1 or 1). The new value is then coerced into the range from 0 to $bound

