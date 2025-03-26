---
title: damp.osc~

description: damped harmonic motion oscillator

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

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  - type: list
    description: frequency and trigger value (normalized to MIDI range)
  2nd:
  - type: float/signal
    description: trigger, determines the maximum amplitude
  3rd:
  - type: float/signal
    description: decay time in ms

outlets:
  1st:
  - type: signal
    description: damped oscillator with cosine initial phase
  2nd:
  - type: signal
    description: damped oscillator with sine initial phase

methods:
  - type: midi <float>
    description: sets to MIDI pitch frequency input

flags:
- name: -midi
  description: sets to MIDI pitch frequency input

draft: false
---
[damp.osc~] is based on a complex pole resonant filter ([vcf2~]) with two ouputs (real/cosine and imaginary/sine). It takes frequency in hertz or midi and a decay time in ms. It is triggered by signals at zero to non zero transitions or by lists at control rate.
