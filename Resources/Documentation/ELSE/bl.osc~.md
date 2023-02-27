---
title: bl.osc~

description: bandlimited oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - type: symbol
    description: (optional) waveform - saw, saw2, tri, square, imp
    default: saw
  - type: float
    description: number of partials
    default: 1
  - type: float
    description: frequency in Hz
    default: 0
  - type: float
    description: phase offset
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
  - type: anything
    description: waveform - saw, saw2, tri, square, imp
  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: bandlimited oscillator signal

methods:
  - type: n <float>
    description: number of partials
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: soft <float>
    description: non-0 sets to soft sync mode

draft: false
---

[bl.osc~] is a bandlimited oscillator based on [wavetable~]. It has wavetables built with a variable number of partials for different waveforms: 'saw', 'saw2', 'tri', 'square' and 'imp'.
