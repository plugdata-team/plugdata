---
title: past

description: check if input increases past a threshold

categories:
 - object

pdcategory: cyclone, Analysis, Data Management

arguments:
- type: float/list
  description: - value(s) of initial threshold(s) (required)
  default:

inlets:
  1st:
  - type: float
    description: - value to check against threshold
  - type: list
    description: - values to check against threshold

outlets:
  1st:
  - type: bang
    description: if input is > threshold value(s) but last input wasn't

methods:
  - type: clear
    description: clears the memory so a next input may trigger a bang
  - type: set <list>
    description: sets threshold to one or more values

draft: true
---

[past] bangs when an input is greater than a threshold value and the last value was below or equal to it. The input needs to fall back to a value equal or below the threshold for another bang to be output when it goes past it again.
