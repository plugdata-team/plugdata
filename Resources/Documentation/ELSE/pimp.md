---
title: pimp

description: control phasor + imp

categories:
- object

pdcategory: ELSE, Signal Generators

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
  2nd:
  - type: bang
    description: output a bang at period transitions

flags:
  - name: -rate <float>
    description: rate period in ms 
    default: 5, min 0.1

methods:
  - type: rate <float>
    description: rate period in ms

draft: false
---

This is like the [pimp~] but operates in a control rate (with a maximum resolution of 1 ms).

