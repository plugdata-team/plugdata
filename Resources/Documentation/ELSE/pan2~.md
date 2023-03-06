---
title: pan2~

description: stereo panning

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: initial pan value
  default: 0

inlets:
  1st:
  - type: signal
    description: signal input
  2nd:
  - type: float/signal
    description: pan value: from -1 (L) to 1 (R)

outlets:
  1st:
  - type: signal
    description: left channel
  2nd:
  - type: signal
    description: right channel

draft: false
---

[pan2~] performs an equal power (sin/cos) panning.

