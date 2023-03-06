---
title: chance~

description: weighted random

categories:
 - object

pdcategory: ELSE, Triggers and Clocks, Random and Noise, Signal Generators

arguments:
- type: list
  description: list of probabilities
  default: 50 50

inlets:
  1st:
  - type: signal
    description: impulse input

outlets:
  nth:
  - type: signal
    description: impulse according to a chance

flags:
  - name: -seed <float>
  description: seed value (default=unique internal)

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

When [chance~] receives an impulse, it outputs it to an outlet according to its chance (probability weight). The total number of chances is the sum of all arguments.
