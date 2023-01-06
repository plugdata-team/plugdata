---
title: cpole~
description: complex one-pole filter
categories:
- object
see_also:
- lop~
- hip~
- bp~
- vcf~
- bob~
- biquad~
- fexpr~
- slop~
pdcategory: Audio Filters
last_update: '0.43'
inlets:
  1st:
  - type: signal
    description: signal to filter (real part).
  - type: set <float, float>
    description: set internal state (real and imaginary part).
  - type: clear
    description: clear internal state to zero (same as "set 0 0").
  2nd:
  - type: signal
    description: signal to filter (imaginary part).
  3rd:
  - type: signal
    description: filter coefficient (real part).
  4th:
  - type: signal
    description: filter coefficient (imaginary part).
outlets:
  1st:
  - type: signal
    description: filtered signal, real part.
  2nd:
  - type: signal
    description: filtered signal, imaginary part.
arguments:
  - type: list
    description: real and imaginary part of coefficient 
  default: 0 0
draft: false
---
Cpole~ filters a complex audio signal (first two inlets