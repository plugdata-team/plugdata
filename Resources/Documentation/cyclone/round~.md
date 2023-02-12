---
title: round~

description: rounds signals

categories:
 - object

pdcategory: cyclone, Signal Math

arguments:
- type: float
  description: value to round to
  default: 0

inlets:
  1st:
  - type: signal
    description: a signal value to round
  2nd:
  - type: float/signal
    description: value to round to (whose multiple values will be approximated to)

outlets:
  1st:
  - type: signal
    description: rounded signal value

flags:
  - name: @nearest
    description: 0 — round, 1 — truncate mode
    default: 0

methods:
  - type: nearest <float>
    description: sets to round (0) or truncate (non-0) mode

draft: true
---

[round~] approximates positive and negative numbers to an integer multiple of any given number that is greater or equal to 0 (0 makes no approximation - so the original input is output unchanged).
It works in two modes, rounding to the nearest multiple (default) or to the approximating to the truncated multiple value.
