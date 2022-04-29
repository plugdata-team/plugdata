---
title: env~
description: envelope follower
categories:
- object
see_also: {}
pdcategory: Audio Filters
last_update: '0.40'
inlets:
  1st:
  - type: signal
    description: signal to be analyzed. 
outlets:
  1st:
  - type: float
    description: RMS envelope in dB. 
arguments:
  - type: float
    description: window size in samples (default 1024).
  - type: float
    description: period in samples per analysis (default half the window size).
draft: false
---
The env~ object takes a signal and outputs its RMS amplitude in dB (with 1 normalized to 100 dB.) Output is bounded below by zero.

The analysis is "Hanning" (raised cosine) windowed.

The optional creation arguments are the analysis window size in samples, and the period (the number of samples between analyses). The latter should normally be a multiple of the DSP block size, although this isn't enforced.