---
title: listfunnel
description: index and output list elements
categories:
 - object
pdcategory: cyclone, Data Management
arguments:
- type: float
  description: index offset
  default: 0
inlets:
  1st:
  - type: anything
    description: any list, the elements are indexed and output
outlets:
  1st:
  - type: list
    description: the elements preceded with an index number, sequentially

methods:
  - type: offset <float>
    description: sets an offset for the index

---

[listfunnel] receives any message and indexes the elements in the format: [index] [element].
