---
title: vcf~
description: voltage-controlled band/lowpass filter
categories:
- object
see_also:
- bp~
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
    description: audio signal to be filtered
  2nd:
  - type: signal
    description: resonant frequency in Hz
  3rd:
  - type: float
    description: set Q
outlets:
  1st:
  - type: signal
    description: real output (bandpasse filtered signal)
  2nd:
  - type: signal
    description: imaginary output (bandpasse filtered signal)
arguments:
  - type: float
    description: initial Q 
    default: 0
methods:
  - type: clear
    description: clear filter's memory
draft: false
---
Vcf~ is a resonant bandpass and lowpass filter that takes either a control or an audio signal to set center frequency, which may thus change continuously in time as in an analog voltage controlled filter (and unlike 'bp~' and 'lop~' that only take control values
