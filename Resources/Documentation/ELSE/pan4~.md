---
title: pan4~
description: quadraphonic panning

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
  - type: float
    description: initial X pan value
    default: 0
  - type: float
    description: initial Y pan value
    default: 0

inlets:
  1st:
  - type: signal
    description: signal input
  2nd:
  - type: float/signal
    description: X pan value: from -1 (Left) to 1 (Right)
  3rd:
  - type: float/signal
    description: Y pan value: from -1 (Back) to 1 (Front)

outlets:
  1st:
  - type: signal
    description: Left-Back channel
  2nd:
  - type: signal
    description: Left-Front channel
  3rd:
  - type: signal
    description: Left-Back channel
  4th:
  - type: signal
    description: Right-Back channel

draft: false
---

[pan4~] is a 4-channel equal power (sin/cos) panner.

