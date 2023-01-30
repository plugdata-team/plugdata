---
title: drunkard~

description: Drunkard's walk algorithm

categories:
- object

pdcategory: ELSE, Random and Noise

arguments:
- description: sets step range
  type: float
  default: 1
- description: sets minimum value
  type: float
  default: -1
- description: sets maximum value
  type: float
  default: 1

inlets:
  1st:
  - type: signal
    description: impulses triggers a random output
  - type: list
    description: two floats set lower and upper bound
  2nd:
  - type: float
    description: sets lower bound
  3rd:
  - type: float
    description: sets upper bound

outlets:
  1st:
  - type: signal
    description: random value as result of the random walk

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)
  - name: -p <float>
    description: sets probability (default 70)

methods:
  - type: step <float>
    description: sets step range
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  - type: p <float>
    description: sets probability of a positive step in %


draft: false
---

[drunkard~] generates random values within a given step range from the last output. When reaching the bounds, the values get wrapped.

