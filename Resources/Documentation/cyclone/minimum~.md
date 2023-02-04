---
title: minimum~
description: signal minimum of two values
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
- type: float
  description: value to compare minimum with
  default: 0
inlets:
  1st:
  - type: signal
    description: value to compare minimum with argument or right inlet
  2nd:
  - type: float/signal
    description: value to compare minimum with left inlet
outlets:
  1st:
  - type: signal
    description: The minimum value (smaller of the two)

---

[minimum~] outputs a signal which is the minimum of two input signals, or the minimum of an input signal and a given argument.

