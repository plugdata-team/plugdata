---
title: onepole~

description: one-pole lowpass filter

categories:
 - object

pdcategory: cyclone, Filters

arguments:
- type: float
  description: cutoff frequency
  default: 1
- type: symbol
  description: mode (<Hz>, <linear> or <radians>)
  default: Hz

inlets:
  1st:
  - type: signal
    description: signal to filter
  2nd:
  - type: signal
    description: cutoff frequency

outlets:
  1st:
  - type: signal
    description: filtered signal

methods:
  - type: Hz 
    description: sets frequency input mode to 'Hz'
  - type: linear
    description: sets frequency input mode to 'linear'
  - type: radians
    description: sets frequency input mode to 'radians'
  - type: clear
    description: clears filter's memory

draft: true
---

Similar to Pd vanilla's [lop~], [onepole~] implements the simplest of IIR filters, providing a 6dB per octave attenuation.
This filter is very efficient and useful for gently rolling off harsh high end and for smoothing out control signals.
