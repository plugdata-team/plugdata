---
title: maximum~
description: signal maximum of two values
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
- type: float
  description: value to compare maximum with
  default: 0
inlets:
  1st:
  - type: signal
    description: value to compare maximum with argument or right inlet
  2nd:
  - type: float/signal
    description: value to compare maximum with left inlet
outlets:
  1st:
  - type: signal
    description: the maximum value (the greater of the two)

---

[maximum~] outputs a signal which is the maximum of two input signals, or the maximum of an input signal and a given argument.

