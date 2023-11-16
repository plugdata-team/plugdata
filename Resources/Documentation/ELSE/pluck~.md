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
    description: frequency input
  - type: list
    description: set frequency and trigger value (normalized to MIDI range)
  2nd:
  - type: float/signal
    description: trigger (determines the maximum amplitude)
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

methods:
  - type: midi <float>
    description: non zero sets to MIDI pitch input instead of hz.

flags:
  - name: -in
    description: creates an extra right inlet for noise input
  - name: -midi
    description: sets to MIDI pitch input


draft: false
---

[pluck~] is a Karplus-Strong algorithm with a 1st order lowpass filter in the feedback loop. It takes frequency in hertz or midi, a decay time in ms and a cutoff frequency in hz for the filter. It is triggered by signals at zero to non zero transitions or by lists at control rate.

