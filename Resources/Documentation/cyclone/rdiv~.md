---
title: rdiv~, !/~, cyclone/!/~

description: reversed inlets division for signals

categories:
 - object

pdcategory: cyclone, Signal Math

arguments:
- type: float
  description: sets an initial value for the dividend
  default: 0

inlets:
  1st:
  - type: signal
    description: the divisor
  2nd:
  - type: signal
    description: the dividend

outlets:
  1st:
  - type: signal
    description: the division of the two numbers

draft: true
---

[rdiv~] or [!/~] divides a number by the incoming value on the left inlet. Functions like the [/~] object, but the inlets' functions are reversed.
