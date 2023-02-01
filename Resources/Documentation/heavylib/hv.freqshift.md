---
title: hv.freqshift

description: frequency shifter effect unit

categories:
- object

pdcategory: heavylib, Effects

arguments:

inlets:
  1st:
  - type: signal
    description: signal to frequency shift
  2nd:
  - type: float/signal
    description: shift amount in Hz

outlets:
  1st:
  - type: float
    description: frequency shifted signal

draft: false
---

