---
title: rifft~
description: inverse real FFT
categories:
- object
pdcategory: vanilla, Signal Math, Analysis
last_update: '0.33'
see_also:
- block~
- fft~
- ifft~
- rfft~
inlets:
  1st:
  - type: signal
    description: real part of frequency domain signal
  2nd:
  - type: signal
    description: imaginary part of frequency domain signal
outlets:
  1st:
  - type: signal
    description: real part of frequency domain signal
draft: false
---
The FFT objects do Fourier analyses and resyntheses of incoming real or complex signals. Complex signals are handled as pairs of signals (real and imaginary part.) The analysis size is one block. You can use the block~ or switch~ objects to control block size.

The real FFT outputs N/2+1 real parts and N/2-1 imaginary parts. The other outputs are zero. At DC and at the Nyquist there is no imaginary part, but the second through Nth output is as a real and imaginary pair, which can be thought of as the cosine and sin component strengths.

There is no normalization, so that an FFT followed by an IFFT has a gain of N.

See the FFT examples (section "I" of audio examples) to see how to use these in practice.
