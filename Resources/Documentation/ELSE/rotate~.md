---
title: rotate~

description: channel rotation

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of channels (minimum 2)
  default: 2
- type: float
  description: rotation position 
  default: 0

inlets:
  nth:
  - type: signal
    description: channel inputs
  2nd:
  - type: signal
    description: position (from -1 to 1)

outlets:
  nth:
  - type: signal
    description: channel outputs

draft: false
---

[rotate~] performs equal power rotation for 'n' channels (default 2). It takes a position control in which a full cycle is from 0 to 1, negative values from 0 to -1 are wrapped.
