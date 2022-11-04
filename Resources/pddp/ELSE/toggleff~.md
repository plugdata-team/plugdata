---
title: toggleff~


description: Toggle flip flop

categories:
- object

pdcategory: general

arguments:
  1st:
  - type: float
    description: initial ouput <0 or 1>
    default: 0

inlets:
  1st:
  - type: signal
    description: trigger signal

outlets:
  1st:
  - type: signal
    description: 0 and 1 upon receiving a trigger

draft: false
---

[toggleff~] toggles between 0 and 1 upon receiving a trigger. A trigger happens when the signal changes from non-positive to positive.