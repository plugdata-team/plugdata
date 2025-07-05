---
title: gatedelay~

description: delay a gate on value

categories:
- object

pdcategory: ELSE, Signal Math

arguments:
- type: float
  description:  delay time in ms
  default: 0

inlets:
  1st:
  - type: float/signals
    description: gate value
  2nd:
  - type: signals
    description: delay time in ms

outlets:
  1st:
  - type: signals
    description: delayed gate

draft: false
---

[gatedelay~] delays a gate on value, but the gate off is immediately output. The object has support for multichannels
