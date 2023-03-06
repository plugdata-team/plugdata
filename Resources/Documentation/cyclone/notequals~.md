---
title: notequals~, !=~, cyclone/!=~

description: `not equal to` comparison for signals

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
  - type: signal
    description: value used for comparison with left inlet's input

outlets:
  1st:
  - type: signal
    description: 1 or 0

draft: true
---

[notequals~] or [!=~] outputs a 1 signal when the left input is not-equal to the right input or argument and a 0 when it is equal to the right input or argument.
