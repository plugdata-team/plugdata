---
title: cross~
description: crossover filter
categories:
 - object
pdcategory: cyclone, Filters
arguments:
- type: float
  description: cutoff frequency
  default: 1000
inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: signal
    description: cutoff frequency
outlets:
  1st:
  - type: signal
    description: lowpass output
  2nd:
  - type: signal
    description: highpass output

methods:
  - type: clear
    description: clears filter's memory

---

[cross~] is a of 3rd order butterworth crossover filter.

