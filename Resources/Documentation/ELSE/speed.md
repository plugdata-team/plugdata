---
title: speed

description: change bpm speed over amount of beats

categories:
- object

pdcategory: ELSE, Envelopes and LFOs, Triggers and Clocks

arguments:
- type: float
  description: initial bpm
  default: 120


inlets:
  1st:
  - type: float
    description: set bpm immediately
  - type: list
    description: <f,f> — target, number of beats; <f,f,f> — start, target, number of beats

outlets:
  1st:
  - type: float
    description: bpm change
    2nd:
  - type: float
    description: total time in ms


methods:
  - type: set <list>
    description: floats/lists precede by 'set' waits for a bang to trigger
  - type: bang
    description: triggers the object after a 'set' message

draft: false
---

[speed] is somewhat related to [line]. You give it a target bpm value and an amount of time in beats to reach it (speeding up or down). Then, the overall tempo in ms is output (via the right outlet) and bpm values are given for each beat until it reaches the target (left outlet), which correctly emulates an accelerando or ritardando.
