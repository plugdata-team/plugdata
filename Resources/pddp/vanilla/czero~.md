---
title: czero~
description: complex one-zero filter
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
last_update: '0.38'
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
    description: real and imaginary part of coefficient (default 0 0).
draft: false
---
Czero~ filters a complex audio signal (first two inlets) via a raw one-zero (non-recursive) filter, whose coefficients are controlled by creation arguments or by another complex audio signal (remaining two inlets).

The action of czero~ is:

`y[n] = x[n] - a[n] * x[n-1]`

where y[n] is the output, x[n] the input, and a[n] the filter coefficient (all complex numbers). The filter is always stable.

The transfer function is `H(Z) = 1 - aZ^-1`.

Pd also provides a suite of user-friendly filters. This and other raw filters are provided for situations which the user-friendly ones can't handle. See Chapter 8 of http://msp.ucsd.edu/techniques/latest/book-html/node127.html for an introduction to the necessary theory.

