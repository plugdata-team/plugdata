---
title: ping.pong~

description: ping pong delay

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: delay time
  type: float
  default: 0
- description: feedback gain
  type: float
  default: 0

inlets:
  1st:
  - type: signal
    description: input to ping pong delay
  2nd:
  - type: float
    description: delay time in ms
  3rd:
  - type: float
    description: feedback gain

outlets:
  1st:
  - type: signal
    description: ping pong delay output

draft: false
---

[ping.pong~] is a ping pong delay object

