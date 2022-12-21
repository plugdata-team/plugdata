---
title: pulse

description: Control pulse

categories:
- object

pdcategory:

arguments:
- description: frequency in hertz
  type: float
  default: 0
- description: pulse width
  type: float
  default: 0
- description: initial phase offset
  type: float
  default: 0

inlets:
  1st:
  - type: float
    description: frequency in hz
  2nd:
  - type: float
    description: pulse width (from 0 to 1)
  3rd:
  - type: float
    description: phase sync (ressets internal phase)

outlets:
  1st:
  - type: float
    description: toggle output

flags:
  - name: -rate <float>
    description: rate period in ms (default 1, min 0.1)

methods:
  - type: rate <float>
    description: rate period in ms


draft: false
---

This is much like the [pulse~] oscillator but operates in a control rate (with a default resolution of 1 ms) and the output is like a toggle switch.

