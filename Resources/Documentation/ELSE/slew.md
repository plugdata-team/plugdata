---
title: slew

description: control slew limiter

categories:
- object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: slew limit speed
  default: 0
- type: float
  description: initial start value
  default: 0

flags:
- name: -rate <f>
  description: sets refresh rate in ms (default 5, minimum 1)

inlets:
  1st:
  - type: float
    description: values to be slew limited

  2nd:
  - type: float
    description: speed limit

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

[slew] takes an amplitude limit per second and an incoming value to be 'slew limited'. It then generates a line towards the incoming value. The difference between [slew] and [line] is that [slew] has a fixed speed, not a fixed period. A limit of zero stops the line generation and a negative value turns the limiter off.
