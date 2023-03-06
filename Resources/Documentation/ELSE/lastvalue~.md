---
title: lastvalue~

description: report last value

categories:
 - object

pdcategory: ELSE, Analysis

arguments:
- type: float
  description: initial last value
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to detect the last value
  2nd:
  - type: float
    description: set new last value

outlets:
  1st:
  - type: signal
    description: last input value

draft: false
---

[lastvalue~] reports the last value when the signal changes

