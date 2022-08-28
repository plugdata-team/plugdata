---
title: bin.shift~

description: Shift bins

categories:
- object

pdcategory: General Audio Manipulation

arguments:
  1st:
  - description: sets shift number
    type: float
    default: 0
  2nd:
  - description: non-zero sets to wrap mode
    type: float
    default: 0

inlets:
  1st:
  - type: signal
    description: the signal to be shifted
  - type: wrap <float>
    description: non-zero sets to wrap mode
  2nd:
  - type: float
    description: sets number of bins to shift

outlets:
  1st:
  - type: signal
    description: the bin shifted signal

draft: false
---

[bin.shift~] is meant to be used for spectral processing, where you can shift samples (or frequency bins). It is similar to Pd Vanilla's [lrshift~] object, which acts in any block size and shifts samples to the left or right.
