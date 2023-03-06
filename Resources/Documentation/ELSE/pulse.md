---
title: pulse

description: control pulse

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- description: frequency in Hz
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
    description: frequency in Hz
  2nd:
  - type: float
    description: pulse width (from 0 to 1)
  3rd:
  - type: float
    description: phase sync (resets internal phase)

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

