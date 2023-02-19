---
title: greaterthan~, >~, cyclone/>~
description: `is greater than` comparison for signals
categories:
 - object
pdcategory: cyclone, Signal Math, Logic
arguments:
- type: float
  description: value for comparison with left inlet's input
  default: 0
inlets:
  1st:
  - type: signal
    description: value is compared to right inlet's or argument
  2nd:
  - type: float/signal
    description: value for comparison with left inlet
outlets:
  1st:
  - type: signal
    description: 1 or 0 (depending on the result of the comparison)

draft: false
---

[greaterthan~] or [>~] outputs a 1 signal when the left input is greater-than the right input or argument and a 0 when it is less-than or equal-to the right input or argument.

