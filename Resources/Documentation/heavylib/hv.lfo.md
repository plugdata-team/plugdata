---
title: hv.lfo

description: Precision LFOs based on sampled signal oscillators

categories:
- object

pdcategory: heavylib, Envelopes and LFOs

arguments:
- type: symbol
  description: wave shape, <sine, ramp, saw, square or pulse>

inlets:
  1st:
  - type: float
    description: frequency input
  2nd:
  - type: float
    description: phase reset / pulse width

outlets:
  1st:
  - type: float
    description: lfo output between 0 and 1

draft: false
---
