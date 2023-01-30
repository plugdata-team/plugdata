---
title: drunkard

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
  default: 0
- description: sets maximum value
  type: float
  default: 127

inlets:
  1st:
  - type: bang
    description: triggers a random output
  - type: float
    description: sets new current value and outputs it
  2nd:
  - type: float
    description: sets lower bound
  3rd:
  - type: float
    description: sets upper bound

outlets:
  1st:
  - type: float
    description: random number output as result of the random walk

flags:
  - name: -seed <float>
    description:
  - name: -p <float>
    description:

methods:
  - type: step <float>
    description: sets step range
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  - type: set <float>
    description: sets the current value (without output)
  - type: p <float>
    description: sets probability of a positive step in %

draft: false
---

[drunkard] generates random numbers within a given step range from the last output. When reaching the bounds, the values get wrapped.

