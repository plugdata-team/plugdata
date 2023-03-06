---
title: highshelf~
description: highshelf filter

categories:
 - object

pdcategory: ELSE, Filters, Mixing and Routing

arguments:
- type: float
  description: shelving frequency in Hz
  default: 0
- type: float
  description: slope from 0 to 1
  default: 0
- type: float
  description: gain in dB
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float/signal
    description: shelving frequency in Hz
  3rd:
  - type: float/signal
    description: slope (from 0 to 1)
  4th:
  - type: float/signal
    description: gain in dB

outlets:
  1st:
  - type: signal
    description: filtered signal

methods:
  - type: clear
    description: clears filter's memory if you blow it up
  - type: bypass <float>
    description: 1 (bypasses input signal) or 0 (doesn't bypass)

draft: false
---

[highshelf~] is a 2nd order highshelf filter.

