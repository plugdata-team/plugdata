---
title: bin.shift~

description: shift bins

categories:
- object

pdcategory: ELSE, Effects

arguments:
  - description: sets shift number
    type: float
    default: 0
  - description: non-0 sets to wrap mode
    type: float
    default: 0

inlets:
  1st:
  - type: signal
    description: the signal to be shifted
  2nd:
  - type: float
    description: sets number of bins to shift

outlets:
  1st:
  - type: signal
    description: the bin shifted signal

methods:
  - type: wrap <float>
    description: non-0 sets to wrap mode

draft: false
---

[bin.shift~] is meant to be used for spectral processing, where you can shift samples (or frequency bins). It is similar to Pd Vanilla's [lrshift~] object, which acts in any block size and shifts samples to the left or right.
