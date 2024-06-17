---
title: wt2d~

description: two-dimensional wavetable oscillator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
  - type: symbol
    description: array name
    default: none
  - type: float
    description: sets frequency in Hz
    default: 0
  - type: float
    description: sets phase offset
    default: 0

methods:
  - type: set <symbol>
    description: sets an entire array to be used as a waveform
  - type: n <float> <float>
    description: sets number of x and y slices to scan through
  - type: midi <float>
    description: non zero sets to frequency input in MIDI pitch
  - type: soft <flaot>
    description: non zero sets to soft sync mode
  - type: none
    description: sets to no interpolation mode
  - type: lin
    description: sets to linear interpolation mode
  - type: cos
    description: sets to cosine interpolation mode
  - type: lagrange
    description: sets to lagrange interpolation mode
  - type: spline
    description: sets to spline interpolation mode

flags:
  - name: -none
    description: disable interpolation
  - name: -lin
    description: set interpolation mode to linear
  - name: -cos
    description: set interpolation mode to cosine
  - name: -lagrange
    description: set interpolation mode to lagrange
  - name: -n <list>
    description: sets number of x and y slices
  - name: -midi
    description: sets frequency input in MIDI pitch
  - name: -soft
    description: sets to soft sync mode

inlets:
  1st:
  - type: float/signal
    description: sets frequency in hertz
  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)
  4th:
  - type: float/signal
    description: x dimension crossfading input
  5th:
  - type: float/signal
    description: y dimension crossfading input
outlets:
  1st:
  - type: signal
    description: a periodically repeating waveform

draft: false
---

[wt2d~] is an interpolating wavetable oscillator like [wt~], but besides a horizontal dimension, it can also scan through a vertical dimension of a sliced wavetable. As other oscilltors in ELSE, it has input for phase modulation and sync.
