---
title: lace~

description: interleave two or more multichannel signals

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of inlets (at least 2)
  default: 2

inlets:
  nth:
  - type: signal
    description: signal $nth to interleave

outlets:
  1st:
  - type: signals
    description: interleaved signals

draft: false
---

[lace~] interleaves two or more multichannel signals.
