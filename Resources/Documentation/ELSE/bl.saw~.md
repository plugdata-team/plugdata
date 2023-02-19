---
title: bl.saw~

description: bandlimited sawtooth oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description:  frequency in Hz
    type: float
    default: 0
  - description: initial phase offset
    type: float
    default: 0

flags:
  - name: -m
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
    description: sawtooth wave signal

methods:
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: soft <float>
    description: non-0 sets to soft sync mode

draft: false
---

[bl.saw~] is a sawtooth oscillator like [else/saw~], but it is bandlimited.
