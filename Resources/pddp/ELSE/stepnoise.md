---
title: stepnoise

description: Control stepnoise

categories:
- object

pdcategory: Control (Random)

arguments:
- type: float
  description: frequency in hertz
  default: 0

flags:
- name: -seed <f>
  description: sets seed (default: unique internal)

inlets:
  1st:
  - type: float
    description: frequency in hertz up to 100 (negative values accepted)
  - type: seed <f>
    description: a float sets seed, no float sets a unique internal

outlets:
  1st:
  - type: float
    description: Step noise in the range from 0 - 127

draft: false
---

[stepoise] is a control rate stepnoise. It doesn't need the DSP on to function. Give it a seed value if you want a reproducible output.
