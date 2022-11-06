---
title: slew~

description: Control slew limiter

categories:
- object

pdcategory: Control (Ramp, Line Generators, Smoothening)

arguments:
- type: float
  description: slew limit speed
  default: 0

inlets:
  1st:
  - type: float/signal
    description: values to be slew limited
  - type: set <f>
    description: sets new start point and goes back to target
  2nd:
  - type: float
    description: speed limit

outlets:
  1st:
  - type: signal
    description: slew limited values

draft: false
---

[slew~] takes an amplitude limit per second and an incoming value to be 'slew limited'. It then generates a line towards the incoming value. The difference between [slew~] and [glide~] is that [slew~] has a fixed speed, not a fixed period. A limit of zero stops the line generation and a negative value turns the limiter off.
