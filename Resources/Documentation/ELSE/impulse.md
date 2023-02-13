---
title: impulse

description: control impulse

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- description: frequency in Hz
  type: float
  default: 0
- description: initial phase offset
  type: float
  default: 0

inlets:
  1st:
  - type: float
    description: frequency in Hz
  2nd:
  - type: float
    description: phase sync (resets internal phase)

outlets:
  1st:
  - type: bang
    description: bang at period transitions

flags:
  - name: -rate <float>
    description: rate period in ms (default 1, min 0.1)

methods:
  - type: rate <float>
    description: rate period in ms

draft: false
---

This is much like the [impulse~] oscillator but operates in a control rate (with a resolution of 1 ms) and sends bangs. The difference between this and objects like [tempo] or [metro] is that a speed change takes effect right away.

