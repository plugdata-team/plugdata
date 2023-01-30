---
title: rm~

description: ring modulator

categories:
- object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: modulation frequency
  default: 0

inlets:
  1st:
  - type: signal
    description: input signal to be processed
  2nd:
  - type: float
    description: modulation frequency

outlets:
  1st:
  - type: signal
    description: ring modulated signal

draft: false
---

[rm~] is a Ring Modulation abstraction. It takes a modulation frequency input as the argument or via the control right inlet. O modulation frequency of 0 Hz bypasses the input signal.
