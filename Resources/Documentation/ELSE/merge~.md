---
title: merge~

description: merge multichannel signals

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of inlets
  default: 2

inlets:
  nth:
  - type: signals
    description: multichannel signals to merge

outlets:
  1st:
  - type: signals
    description: merged signals


draft: false
---

[merge~] merges different signals of different channel lenghts. The number of inlets is set via the argument.