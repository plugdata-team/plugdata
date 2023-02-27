---
title: cosine~

description: cosine oscillator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: initial phase offset
  default: 0

flags:
  - name: -midi
    description: sets frequency input in MIDI pitch (default Hz)
  - name: -soft
    description: sets to soft sync mode (default hard)

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: cosine wave signal

methods:
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: soft <float>
    description: non-0 sets to soft sync mode

draft: false
---

[cosine~] is a sinusoidal oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation.
