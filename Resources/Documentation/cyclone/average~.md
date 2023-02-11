---
title: average~
description: signal moving average
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
- type: float
  description: maximum and initial 'n' samples (minimum 1)
  default: 100
- type: symbol
  description: sets mode: bipolar, absolute, or rms
  default: bipolar
inlets:
  1st:
  - type: signal
    description: the signal to be averaged
  - type: float
    description: number of last samples to apply the average to
outlets:
  1st:
  - type: signal
    description: the moving average over the last 'n' samples

methods:
  - type: bipolar
    description: outputs average over positive and negative values
  - type: absolute
    description: outputs average over absolute values
  - type: rms
    description: outputs 'root mean square' (RMS) values

---

Use [average~] for a signal running/moving average over the last 'n' given samples. The average is done in 3 modes: bipolar (default), absolute and rms.

