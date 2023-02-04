---
title: rms~

description: detect RMS amplitude

categories:
 - object

pdcategory: ELSE, Analysis

arguments:
- type: float
  description: analysis window size in samples
  default: 1024
- type: float
  description: hop size in samples
  default: half the window size

inlets: 
  1st:
  - type: signal
    description: the signal to analyze
  - type: set <f,f>
    description: sets window and hop size in samples
  - type: db
    description: change RMS value to dBFS
  - type: linear
    description: change RMS value to linear

outlets:
  1st:
  - type: float
    description: RMS value

draft: false
---

[rms~] is similar to Pd Vanilla's [env~], but it reports the RMS value in linear amplitude (default) or in dBFS.
