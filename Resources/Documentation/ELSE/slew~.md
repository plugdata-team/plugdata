---
title: slew~

description: control slew limiter

categories:
- object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: slew limit speed
  default: 0

inlets:
  1st:
  - type: float/signal
    description: values to be slew limited
  2nd:
  - type: float
    description: speed limit

outlets:
  1st:
  - type: signal
    description: slew limited values

methods:
  - type: set <float>
    description: sets new start point and goes back to target

draft: false
---

[slew~] takes an amplitude limit per second and an incoming value to be 'slew limited'. It then generates a line towards the incoming value. The difference between [slew~] and [glide~] is that [slew~] has a fixed speed, not a fixed period. A limit of zero stops the line generation and a negative value turns the limiter off.
