---
title: crossover~

description: Crossover filter

categories:
 - object

pdcategory: DSP (Filters)

arguments:
- type: float
  description: cutoff frequency
  default: 1000

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  - type: clear
    description: clears filter's memory
  2nd:
  - type: float/signal
    description: cutoff frequency

outlets:
  1st:
  - type: signal
    description: lowpass output
  2nd:
  - type: signal
    description: highpass output

draft: false
---

[crossover~] is a of 3rd order butterworth crossover filter.
It has two outlets for lowpass (left) and highpass (right) filters that you can use separately or in combination to form a crossover filter.