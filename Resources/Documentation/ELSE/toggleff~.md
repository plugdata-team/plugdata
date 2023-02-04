---
title: toggleff~


description: toggle flip flop

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
  - type: float
    description: initial output <0 or 1>
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
