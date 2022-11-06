---
title: sum

description: Sum a list

categories:
- object

pdcategory: List Management 

arguments: (none)

inlets:
  1st:
  - type: bang
    description: sums the last received list
  - type: list
    description: list of floats to sum

outlets:
  1st:
  - type: float
    description: summed value of input list

draft: false
---

[sum] sums a list of values.
