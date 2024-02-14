---
title: bl.wavetable~

description: bandlimited wavetable oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: array name (required)
    type: symbol
    default: none
  - description: sets frequency in Hz
    type: float
    default: 0
  - description: sets phase offset
    type: float
    default: 0

flags:
  - name: -none/-lin/-cos/-lagrange
    description: set interpolation mode
    default: spline
  - name: -midi
    description: sets frequency input in MIDI pitch
    default: Hz
  - name: -soft
    description: sets to soft sync mode
    default: hard

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
    description: a periodically repeating waveform

methods:
  - type: set <symbol>
    description: sets an entire array to be used as a waveform
  - type: size <float>
    description: sets size in number of points
  - type: offset <float>
    description: sets offset in table
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: soft <float>
    description: non-0 sets to soft sync mode
  - type: none
    description: sets to no interpolation mode
  - type: lin
    description: sets to linear interpolation mode
  - type: cos
    description: sets to cosine interpolation mode
  - type: lagrange
    description: sets to Lagrange interpolation mode
  - type: spline
    description: sets to spline interpolation mode (default)
    

draft: false
---

[bl.wavetable~] is a wavetable oscillator like [else/wavetable~], but it is bandlimited with the upsampling/filtering technique. This makes the object quite inefficient CPU wise, but is an easy way to implement a bandlimited oscillator. This object also minimizes aliasing caused by hard sync and phase modulation.
