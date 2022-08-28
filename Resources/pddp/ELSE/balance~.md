---
title: balance~

description: Stereo balancer

categories:
 - object

pdcategory: General Audio Manipulation

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
    description:
  2nd:
  - type: signal
    description:

draft: false
---

[balance~] performs an equal power (sin/cos) stereo balancing.