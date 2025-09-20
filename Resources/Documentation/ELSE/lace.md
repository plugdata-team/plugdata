---
title: lace

description: interleave lists

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: number of inlets (at least 2)
  default: 2

inlets:
  nth:
  - type: list
    description: list $nth to be interleaved

outlets:
  1st:
  - type: list
    description: the interleaved lists

draft: false
---

[lace] interleaves two or more lists. A bang is considered an empty list.
