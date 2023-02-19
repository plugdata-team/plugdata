---
title: pulsecount~
description: pulse counter

categories:
 - object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: float
  description: maximum count value
  default: 0 - no maximum

inlets:
  1st:
  - type: signal
    description: trigger signal to count
  2nd:
  - type: signal
    description: an impulse resets counter to zero

outlets:
  1st:
  - type: signal
    description: the trigger count

methods:
  - type: max <float>
    description: sets maximum count value

draft: false
---

[pulsecount~] counts pulse triggers. A trigger happens at transitions from non positive to positive. A trigger in the right inlet resets the counter to zero. The argument sets a maximum count value (if greater than 0).

