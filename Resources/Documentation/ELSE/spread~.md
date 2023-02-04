---
title: spread~

description: spread inputs to outputs

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: 'n' number of inputs (2 to 512)
  default: 2
- type: float
  description: 'n' number of outputs (2 to 512)
  default: 2

inlets:
  nth:
  - type: signal
    description: input channels

outlets:
  nth:
  - type: signal
    description: output channels

draft: false
---

[spread~] spreads any number of input channels across any number of output channels with equal power panning.
