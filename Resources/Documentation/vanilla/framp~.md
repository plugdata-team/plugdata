---
title: framp~
description: estimate frequency and amplitude of FFT
categories:
- object
pdcategory: vanilla, Signal Math, Analysis
last_update: '0.47'
inlets:
  1st:
  - type: signal
    description: real part of FFT
  2nd:
  - type: signal
    description: imaginary part of FFT
outlets:
  1st:
  - type: signal
    description: estimated frequency of bin
  2nd:
  - type: signal
    description: estimated amplitude of bin
draft: false
---
Framp~ takes as input a rectangular-windowed FFT and outputs, for each FFT channel, the estimated amplitude and frequency of any component feeding that channel. A sinusoidal component should appear in four components (or three in the special case of a sinusoid exactly tuned to a bin.) Frequency output is in bins, i.e., units of SR/N.

The estimation is done according to the hop-1 trick described in Puckette&Brown, Accuracy of Frequency Estimates Using the Phase Vocoder, using a Hann window. A more sophisticated version of this technique is used in the sigmund~ object.
