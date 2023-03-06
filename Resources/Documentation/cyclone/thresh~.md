---
title: thresh~

description: detect signal above threshold

categories:
 - object

pdcategory: cyclone, Analysis

arguments:
- type: float
  description: sets low threshold
  default: 0
- type: float
  description: sets high threshold
  default: 0

inlets:
  1st:
  - type: signal
    description: a signal whose level you want to detect
  2nd:
  - type: float/signal
    description: low threshold level
  3rd:
  - type: float/signal
    description: high threshold level

outlets:
  1st:
  - type: signal
    description: 1/0 depending on signal detect

draft: true
---

[thresh~] is a "Schmitt trigger". When the input is greater than or equal to the high threshold level, the output is 1 and becomes 0 when the signal is equal to or less than the reset level (low threshold).
If low and high threshold are the same, the output is 1 until a sample in the input signal is less than the reset level (thus it works in the same way as [>=~]).
