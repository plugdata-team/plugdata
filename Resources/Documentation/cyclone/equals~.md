---
title: equals~, ==~, cyclone/==~
description: `is equal to` comparison for signals
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
    description: value used for comparison with left inlet's input
outlets:
  1st:
  - type: signal
    description: 1 or 0 (depending on the result of the comparison)

draft: false
---

[equals~] or [==~] outputs a signal that is "1" when the left input is equal to the right input/argument or "0" when it isn't.

