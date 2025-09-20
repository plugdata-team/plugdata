---
title: delace~

description: Deinterleave a multichannel signal

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of outlets (at least 2)
  default: 2

inlets:
  1st:
  - type: signals
    description: multichannel signal to deinterleave

outlets:
  nth:
  - type: signals
    description: deinterleaved signal $nth

draft: false
---

[delace~] deinterleaves a multichannel signal according to the number of outlets.
