---
title: phasor

description: control phasor

categories:
- object

pdcategory: ELSE, Triggers and Clocks, Envelopes and LFOs

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
  - type: float
    description: "phase" value from 0 to 127

flags:
  - name: -rate <float>
    description: rate period in ms (default 1, min 0.1)

methods:
  - type: rate <float>
    description: rate period in ms

draft: false
---

This is like the [phasor~] but operates in a control rate (with a maximum resolution of 1 ms).

