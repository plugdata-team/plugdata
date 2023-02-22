---
title: peak~

description: detect peak amplitude

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
    description: signal to analyze

outlets:
  1st:
  - type: float
    description: peak amplitude value

flags:
  - name: -db
    description: sets the output to dBFS

methods:
  - type: set <float, float>
    description: sets window and hop size in samples
  - type: db
    description: change peak value to dBFS
  - type: linear
    description: change peak value to linear (the default is linear)

draft: false
---

[peak~] is similar to Pd Vanilla's [env~], but it reports the peak amplitude value in linear (default) or dBFS.

