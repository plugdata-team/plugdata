---
title: rotate.mc~

description: multichannel rotation

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: rotation position
  default: 0

inlets:
  1st:
  - description: multichannel input
    type: signals
  2nd:
  - description: position (from -1 to 1)
    type: float/signal


outlets:
  1st:
  - description: multichannel output
    type: signal

draft: false
---

[rotate.mc~] performs equal power rotation for any number of channels given by a multichannel signal input. It takes a position control in which a full cycle is from 0 to 1, negative values from 0 to -1 are wrapped.
