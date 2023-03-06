---
title: svf~

description: state variable filter

categories:
 - object

pdcategory: cyclone, Filters

arguments:
- type: float
  description: cutoff/center frequency in Hz
  default:
- type: float
  description: Q/resonance (0-1)
  default: 0.01
- type: symbol
  description: frequency mode (Hz, linear, radians)
  default: Hz

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: signal
    description: cutoff/center frequency in Hz
  3rd:
  - type: signal
    description: Q/resonance (0-1)

outlets:
  1st:
  - type: signal
    description: lowpass filter signal
  2nd:
  - type: signal
    description: highpass filter signal
  3rd:
  - type: signal
    description: bandpass filter signal
  4th:
  - type: signal
    description: notch filter signal

methods:
 - type: clear
   description: clears the filter in case of a blow-up
 - type: hz
   description: sets frequency mode to Hz 
 - type: linear
    description: sets frequency mode to linear
  - type: radians
    description: sets frequency mode to radians

draft: true
---

[svf~] implements Chamberlin's state-variable filter algorithm, which outputs lowpass, highpass, bandpass, and band reject (notch) simultaneously in parallel (in this order from left to right).
