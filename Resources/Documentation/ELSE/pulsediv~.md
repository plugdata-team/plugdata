---
title: pulsediv~
description: pulse divider

categories:
 - object

pdcategory: ELSE, Signal Generators, Triggers and Clocks

arguments:
- type: float
  description: divide value
  default: 1
- type: float
  description: start count value
  default: 0

inlets:
  1st:
  - type: signal
    description: trigger signal to be divided
  2nd:
  - type: signal
    description: an impulse resets the counter to the start value
outlets:
  1st:
  - type: signal
    description: divided triggers (impulse)
  2nd:
  - type: signal
    description: impulse for the other trigger inputs

draft: false
---

[pulsediv~] outputs impulses when receiving triggers (signal changes from non-positive to positive). The left outlet outputs impulses according to the division value and the right outlet outputs impulses for the other trigger inputs.

