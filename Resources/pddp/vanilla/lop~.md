---
title: lop~
description: one-pole low pass filter.
categories:
- object
see_also:
- hip~
- bp~
- vcf~
- bob~
- biquad~
- slop~
- cpole~
- fexpr~
pdcategory: Audio Filters
last_update: '0.38'
inlets:
  1st:
  - type: signal
    description: audio signal to be filtered.
  - type: clear
    description: clear filter's memory.
  2nd:
  - type: float
    description: rolloff frequency.	
outlets:
  1st:
  - type: signal
    description: filtered signal. 
arguments:
  - type: float
    description: rolloff frequency in Hz (default 0).
draft: false
---
lop~ is a one-pole low pass filter with a specified rolloff frequency. The left inlet is the incoming audio signal. The right inlet is the cutoff frequency in Hz.