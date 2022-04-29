---
title: vcf~
description: voltage-controlled band/low-pass filter
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
pdcategory: Audio Filters
last_update: '0.46'
inlets:
  1st:
  - type: signal
    description: audio signal to be filtered.
  - type: clear
    description: clear filter's memory.
  2nd:
  - type: signal
    description: resonant frequency in Hz.
  3rd:
  - type: float
    description: set Q.
outlets:
  1st:
  - type: signal
    description: real output (bandpasse filtered signal). 
  2nd:
  - type: signal
    description: imaginary output (bandpasse filtered signal).
arguments:
  - type: float
    description: initial Q (default 0).
draft: false
---
Vcf~ is a resonant band-pass and low-pass filter that takes either a control or an audio signal to set center frequency, which may thus change continuously in time as in an analog voltage controlled filter (and unlike 'bp~' and 'lop~' that only take control values). The "Q" or filter sharpness is still only set by control messages. It is more expensive but more powerful than the bp~ bandpass filter.

Vcf~ is implemented as a one-pole complex filter with outlets for the real and imaginary value. These may be used as bandpass and lowpass filter outputs, or combined to allow other possibilities.