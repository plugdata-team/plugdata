---
title: changed~

description: detect signal changes

categories:
 - object

pdcategory: ELSE, Analysis, Triggers and Clocks

arguments:
- type: float
  description: threshold
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to detect changes
  2nd:
  - type: signal
    description: threshold value

outlets:
  1st:
  - type: signal
    description: impulse when a change is detected

draft: false
---

[changed~] sends an impulse whenever an input value changes and the change is equal or greater than a given threshold. If the threshold is 0, then any change is detected.
