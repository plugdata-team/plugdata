---
title: biquad~
description: 2nd order (2-pole / 2-zero) filter
categories:
- object
see_also:
- lop~
- hip~
- bp~
- vcf~
- bob~
- fexpr~
- slop~
- rpole~
- rzero~
- rzero_rev~
- cpole~
- czero~
- czero_rev~
pdcategory: Audio Filters
last_update: '0.30'
inlets:
  1st:
  - type: signal
    description: input signal to be filtered.
  - type: list
    description: input signal to be filtered (fb1 fb2 ff1 ff2 ff3).
  - type: set <float, float>
    description: set last two input samples.
  - type: clear
    description: clear filter's memory buffer.
outlets:
  1st:
  - type: signal
    description: the filtered signal output.
arguments:
  - type: list
    description: initializes the 5 coefficients (fb1 fb2 ff1 ff2 ff3).
draft: false
---
Biquad~ calculates the following difference equation:

`y(n) = ff1 * w(n) + ff2 * w(n-1) + ff3 * w(n-2)`

`w(n) = x(n) + fb1 * w(n-1) + fb2 * w(n-2)`

The filter's coefficients syntax (set via arguments or list input) is: fb1 fb2 ff1 ff2 ff3