---
title: schmitt

description: Schmitt trigger

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: float
  description: low threshold
  default: 0
- type: float
  description: high threshold
  default: 1

inlets:
  1st:
  - type: float
    description: values to analyze
  2nd:
  - type: float
    description: low threshold level
  3rd:
  - type: float
    description: high threshold level

outlets:
  1st:
  - type: float
    description: 1/0 depending on the analysis

draft: false
---

[schmitt] is a Schmitt trigger. When the input is greater than or equal to the high threshold level, the output is 1 and becomes 0 when the signal is equal to or less than the reset level (low threshold).
