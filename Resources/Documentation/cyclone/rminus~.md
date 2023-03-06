---
title: rminus~, !-~, cyclone/!-~

description: reversed inlets subtraction for signals

categories:
 - object

pdcategory: cyclone, Signal Math

arguments:
- type: float
  description: sets an initial value to subtract from
  default: 0

inlets:
  1st:
  - type: signal
    description: the subtrahend (hot inlet)
  2nd:
  - type: signal
    description: the minuend (cold inlet)

outlets:
  1st:
  - type: signal
    description: the difference of the two numbers

draft: true
---

[rminus~] or [!-~] is like the [-~] object, but the inlets' functions are reversed.
