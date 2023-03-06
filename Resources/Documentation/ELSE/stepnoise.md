---
title: stepnoise

description: control stepnoise

categories:
- object

pdcategory: ELSE, Random and Noise

arguments:
- type: float
  description: frequency in Hz
  default: 0

flags:
- name: -seed <f>
  description: sets seed (default: unique internal)

inlets:
  1st:
  - type: float
    description: frequency in Hz up to 100 (negative values accepted)

outlets:
  1st:
  - type: float
    description: Step noise in the range from 0 - 127

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[stepoise] is a control rate stepnoise. It doesn't need the DSP on to function. Give it a seed value if you want a reproducible output.
