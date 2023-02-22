---
title: pluck~
description: a Karplus-Strong algorithm
categories:
 - object
pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: decay time in ms
  default: 0
- type: float
  description: filter cutoff frequency
  default: around 7020

inlets:
  1st:
  - type: signal
    description: trigger (determines the amplitude)
  - type: float
    description: non-0 triggers and sets amplitude
  - type: bang
    description: triggers with last control amplitude (default 1)
  2nd:
  - type: float/signal
    description: frequency in Hz (minimum 1)
  3rd:
  - type: float/signal
    description: decay time in ms
  4th:
  - type: float/signal
    description: filter cutoff frequency
  5th:
  - type: float/signal
    description: optional noise input (with the -in flag)

outlets:
  1st:
  - type: signal
    description: the Karplus-Strong output

flags:
  - name: -in
    description: creates an extra right inlet for noise input

draft: false
---

[pluck~] is a Karplus-Strong algorithm with a 1st order lowpass filter in the feedback loop. It takes frequency in Hz, a decay time in ms and a cutoff frequency for the filter. It is triggered by signals at zero to non-0 transitions or by floats and bangs at control rate.

