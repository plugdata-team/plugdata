---
title: pvoc.live~

description: pitch-shift time-stretch

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: sets buffer size in ms
  type: float
  default: 5000
- description: sets playing speed
  type: float
  default: 100
- description: sets transposition in cents
  type: float
  default: 0

inlets:
  1st:
  - type: signal
    description: audio signal input
  - type: bang
    description: resets to the beginning of the delay line
  2nd:
  - type: float
    description: sets speed
  3rd:
  - type: float
    description: sets transposition in cents

outlets:
  1st:
  - type: signal
    description: processed output

draft: false
---

[pvoc.live~] is like [pvoc.player~], but for live input. It provides independent time stretching and pitch shifting via granulation.

