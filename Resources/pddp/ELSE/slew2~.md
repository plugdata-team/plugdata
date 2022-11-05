---
title: slew2~

description: Control slew limiter

categories:
 - object

pdcategory: Control (Ramp, Line Generators, Smoothening)

arguments:
- type: float
  description: up slew limit speed
  default: 0
- type: float
  description: down slew limit speed
  default: 0

inlets:
  1st:
  - type: float/signal
    description: values to be slew limited
  - type: set <f>
    description: sets new start point and goes back to target
  2nd:
  - type: float/signal
    description: speed limit upwards
  3rd:
  - type: float
    description: speed limit downwards 

outlets:
  1st:
  - type: float
    description: slew limited values

draft: false
---

[slew2~] is like [slew~] but has independent values for upwards & downwards ramps. It takes an amplitude limit per second and generates a line towards the incoming value. The difference between [slew~] and [glide~] is that [slew~] has a fixed speed, not a fixed period. A limit of zero stops the line generation and a negative value turns the limiter off (but again, up and down limits are independent).
