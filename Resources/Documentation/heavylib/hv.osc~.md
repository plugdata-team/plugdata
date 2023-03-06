---
title: hv.osc~

description: polynomial band-limited polystep oscillators

categories:
- object

pdcategory: heavylib, Signal Generators

arguments:
- type: symbol
  description: wave shape, <sine, saw, or square>

inlets:
  1st:
  - type: signal
    description: frequency input
  2nd:
  - type: float
    description: phase reset

outlets:
  1st:
  - type: signal
    description: oscillator output

draft: false
---

