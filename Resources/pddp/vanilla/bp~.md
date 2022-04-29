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
pdcategory: Audio Filters
last_update: '0.46'
inlets:
  1st:
  - type: signal
    description: input signal to be filtered. 
  - type: clear
    description: clear filter's memory.
  2nd:
  - type: float
    description: center frequency in Hz.
  3rd:
  - type: float
    description:  Q (controls bandwidth).  	
outlets:
  1st:
  - type: signal
    description: the filtered signal output.
arguments:
  - type: float
    description: initial center frequency in Hz (default 0).
  - type: float
    description: initial Q (default 0).
draft: false
---
bp~ passes a sinusoid at the center frequency at unit gain (approximately). Other frequencies are attenuated.

NOTE: the maximum center frequency is sample rate divided by 6.28, or about 12kHz at the "usual" rates - for a more general and stable (but slightly more CPU-expensive) filter, try vcf~.