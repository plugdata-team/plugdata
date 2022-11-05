---
title: svfilter~

description: State-variable pedal

categories:
 - object
  
pdcategory: DSP (Filters) 

arguments:
  - type: float
    description: cutoff/center frequency (Hz)
	default:
  - type: float
    description: sets Q/resonance from 0-1
    default: 0.01

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  - type: clear
    description: clears the filter in case of a blow-up
  2nd:
  - type: float/signal
    description: sets cutoff/center frequency
  3rd:
  - type: float/signal
    description: sets Q/resonance (0-1)

outlets:
  1st:
  - type: signal
    description: lowpass filter signal
  2nd:
  - type: signal
    description: lowpass filter signal
  3rd:
  - type: signal
    description: bandpass filter signal
  4th:
  - type: signal
    description: notch filter signal

draft: false
---

[svfilter~] implements Chamberlin's state-variable filter algorithm, which outputs lowpass, highpass, bandpass, and bandstop simultaneously in parallel (in this order from left to right).
