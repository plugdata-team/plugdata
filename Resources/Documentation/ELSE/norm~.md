---
title: norm~

description: normalizer

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: normalize level in dBFS
  type: float
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be normalized
  2nd:
  - type: float
    description: normalize level in dBFS

outlets:
  1st:
  - type: signal
    description: normalized signal

flags:
  - name: -size <float>
    description: sets analysis window size in samples (default 1024)

draft: false
---

[norm~] is a normalizer abstraction based on [mov.rms~]. It takes a normalization value in dBFS and a window analysis size.
