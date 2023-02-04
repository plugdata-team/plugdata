---
title: crossover~

description: crossover filter

categories:
 - object

pdcategory: ELSE, Filters

arguments:
- type: float
  description: cutoff frequency
  default: 1000

inlets:
  1st:
  - type: signal
    description: signal to be filtered
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

methods:
  - type: clear
    description: clears filter's memory

draft: false
---

[crossover~] is a of 3rd order butterworth crossover filter.
It has two outlets for lowpass (left) and highpass (right) filters that you can use separately or in combination to form a crossover filter.
