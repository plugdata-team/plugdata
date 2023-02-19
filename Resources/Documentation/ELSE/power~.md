---
title: power~

description: power function waveshaper

categories:
 - object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: exponential factor
  default: 1

inlets:
  1st:
  - type: float/signal
    description: signal input
  2nd:
  - type: float/signal
    description: exponential factor

outlets:
  1st:
  - type: signal
    description: the signal raised to the given power

draft: false
---

[power~] is a power function waveshaper for signals that extends the usual definition of exponentiation and returns -pow(-a, b) when you have a negative signal input. This allows not only the output of negative values but also the exponentiation of negative values by non-integer exponents.

