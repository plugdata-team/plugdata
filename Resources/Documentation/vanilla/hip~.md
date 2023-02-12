---
title: hip~
description: one-pole highpass filter
categories:
- object
see_also:
- lop~
- bp~
- vcf~
- bob~
- biquad~
- slop~
- cpole~
- fexpr~
pdcategory: vanilla, Filters
last_update: '0.44'
inlets:
  1st:
  - type: signal
    description: audio signal to be filtered
  - type: clear
    description: clear filter's memory
  2nd:
  - type: float
    description: rolloff frequency
outlets:
  1st:
  - type: signal
    description: filtered signal
arguments:
  - type: float
    description: rolloff frequency in Hz 
  default: 0
draft: false
---
hip~ is a one-pole highpass filter with a specified cutoff frequency. Left (audio
