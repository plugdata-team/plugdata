---
title: hv.vline~

description: [line~] based replacement for [vline~]

categories:
- object

pdcategory: heavylib, Signal Generators, Envelopes and LFOs

arguments:

inlets:
  1st:
  - type: float
    description: jump to value
  - type: list
    description: <target value, time interval in ms>

outlets:
  1st:
  - type: signal
    description: ramp output

methods:
  - type: stop
    description: freeze vline at its current value

draft: false
---

