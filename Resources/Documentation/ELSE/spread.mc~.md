---
title: spread.mc~

description: spread signal input to a multichannel output

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of output channels
  default: 2

inlets:
  1st:
  - description: multichannel input
    type: signals

outlets:
  1st:
  - description: multichannel output
    type: signals

methods:
  - type: n <float>
    description: set number of output channels

draft: false
---

[spread.mc~] spreads any number of input channels across any number of output channels with equal power panning. It takes and outputs a multichannel signal.