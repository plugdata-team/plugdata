---
title: slew2

description: control slew limiter

categories:
- object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: up slew limit speed
  default: 0
- type: float
  description: down slew limit speed
  default: 0

flags:
- type: -rate <float>
  description: sets refresh rate in ms (default 5, minimum 1)

inlets:
  1st:
  - type: float
    description: values to be slew limited
  2nd:
  - type: float
    description: up slew speed limit
  3rd:
  - type: float
    description: down slew speed limit

outlets:
  1st:
  - type: float
    description: slew limited values

methods:
  - type: set <float>
    description: sets new start point and goes back to target
  - type: rate <float>
    description: refresh rate in ms

draft: false
---

[slew2] is a slew limiter like [slew], but has a different speed for up and down changes. It takes an amplitude limit per second and an incoming value to be 'slew limited'. It then generates a line towards the incoming value. The difference between [slew] and [line] is that [slew] has a fixed speed, not a fixed period. A limit of zero stops the line generation and a negative value turns the limiter off.
