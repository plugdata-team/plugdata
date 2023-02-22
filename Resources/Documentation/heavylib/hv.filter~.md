---
title: hv.filter~

description: generic filter module with swappable filter types and frequency, Q settings

categories:
- object

pdcategory: heavylib, Filters

arguments:
- type: symbol
  description: filter type <allpass, lowpass, highpass, bandpass1, bandpass2, or notch>
- type: float
  description: frequency
- type: float
  description: Q

inlets:
  1st:
  - type: signal
    description: input signal
  2nd:
  - type: float
    description: frequency
  3rd:
  - type: float
    description: Q

outlets:
  1st:
  - type: signal
    description: filtered signal

draft: false
---

