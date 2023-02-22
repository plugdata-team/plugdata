---
title: bp~
description: 2-pole bandpass filter
categories:
- object
see_also:
- vcf~
- bob~
- lop~
- hip~
- biquad~
- slop~
- cpole~
- fexpr~
pdcategory: vanilla, Filters
last_update: '0.46'
inlets:
  1st:
  - type: signal
    description: input signal to be filtered
  2nd:
  - type: float
    description: center frequency in Hz
  3rd:
  - type: float
    description:  Q (controls bandwidth)
outlets:
  1st:
  - type: signal
    description: the filtered signal output
arguments:
  - type: float
    description: initial center frequency in Hz 
  default: 0
  - type: float
    description: initial Q 
methods:
  - type: clear
    description: clear filter's memory
draft: false
---
