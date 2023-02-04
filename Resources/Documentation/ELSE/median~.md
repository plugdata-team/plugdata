---
title: median~

description: signal median

categories:
 - object

pdcategory: ELSE, Analysis, Signal Math

arguments:
- type: float
  description: number of samples to perform the median
  default: 1

inlets:
  1st:
  - type: signal
    description: the signal to perform the median on
  2nd:
  - type: float
    description: number of samples to perform the median

outlets:
  1st:
  - type: signal
    description: the median of the input signal


draft: false
---

The [median~] objects returns the median of a number of samples (minimum number of samples is 1 and maximum is the block size).
