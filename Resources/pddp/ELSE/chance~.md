---
title: chance~

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
  - type: signal
    description: a bang to be passed or not
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

outlets:
  Nth:
  - type: signal
    description: impulse according to a chance

flags:
  - name: -seed <float>
  description: seed value (default=unique internal)

draft: false
---

When [chance~] receives an impulse, it outputs it to an outlet according to its chance (probability weight). The total number of chances is the sum of all arguments.