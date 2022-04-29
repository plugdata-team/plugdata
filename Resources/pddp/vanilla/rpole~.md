---
title: rpole~
description: real one-pole filter.
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
last_update: '0.42'
inlets:
  1st:
  - type: signal
    description: real signal to filter.
  - type: set <float>
    description: set internal state.
  - type: clear
    description: clear internal state to zero (same as "set 0").
  2nd:
  - type: signal
    description: filter coefficient.
outlets:
  1st:
  - type: signal
    description: filtered signal.
arguments:
  - type: float
    description: filter coefficient (default 0).
draft: false
---
Rpole~ filters an audio signal (left inlet) via a raw one-pole (recursive) real filter, whose coefficient is controlled by a creation argument or by an audio signal (right inlet).

The action of rpole~ is:

`y[n] = x[n] + a[n] * y[n-1]`

where y[n] is the output, x[n] the input, and a[n] the filter coefficient. The filter is unstable if/when `|a[n]|>1`.

The transfer function is `H(Z) = 1/(1 - aZ^-1)`.

Pd also provides a suite of user-friendly filters. This and other raw filters are provided for situations which the user-friendly ones can't handle. See Chapter 8 of http://msp.ucsd.edu/techniques/latest/book-html/node127.html for an introduction to the necessary theory.

