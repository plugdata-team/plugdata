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

flags:
 - name: -size <float>
   description: sets buffer size
 - name: -lin
   description: sets output to linear
   default: dBFS

methods:
  - type: set <float, float>
    description: sets window and hop size in samples
  - type: lin
    description: non zero sets to linear output

outlets:
  1st:
  - type: float
    description: RMS value

draft: false
---

[rms~] is similar to Pd Vanilla's [env~], but it reports the RMS value in linear amplitude (default) or in dBFS.
