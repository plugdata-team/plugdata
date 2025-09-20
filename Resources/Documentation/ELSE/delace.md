---
title: delace

description: deinterleave lists

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: number of outlets (at least 2)
  default: 2

inlets:
  1st:
  - type: list
    description: lists to be deinterleaved

outlets:
  nth:
  - type: list
    description: deinterleaved list $nth

draft: false
---

[delace] deinterleaves a list according to the number of outlets.
