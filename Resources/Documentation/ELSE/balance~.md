---
title: balance~

description: stereo balancer

categories:
 - object

pdcategory: ELSE, Mixing and Routing, Effects

arguments:
- type: float
  description: initial balance value
  default: 0

inlets:
  1st:
  - type: signal
    description: left signal input
  2nd:
  - type: signal
    description: right signal input
  3rd:
  - type: float/signal
    description: balance value = from -1 (L) to 1 (R)

outlets:
  1st:
  - type: signal
    description: left channel
  2nd:
  - type: signal
    description: right channel

draft: false
---

[balance~] performs an equal power (sin/cos) stereo balancing.
