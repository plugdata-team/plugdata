---
title: wavetable~, wt~

description: wavetable oscillator

categories:
 - object

pdcategory: ELSE, Signal Generators, Arrays and Tables, Buffers

arguments:
  - type: symbol
    description: array name (optional)
    default: none
  - type: float
    description: sets frequency in Hz
    default: 0
  - type: float
    description: sets phase offset
    default: 0
  - type: float
    description: sets crossfading
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
  - type: list/signals
    description: sets frequency in Hz

  2nd:
  - type: float/signals
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signals
    description: phase offset (modulation input)
  4th:
  - type: float/signals
    description: crossfading input (0-1)

outlets:
  1st:
  - type: signals
    description: a periodically repeating waveform

methods:
  - type: table <symbol>
    description: sets an entire array to be used as a waveform
  - type: set <float, float>
    description: sets a single frequency channel
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

[wavetable~] is an interpolating wavetable oscillator like Pd Vanilla's [tabosc4~]. It accepts negative frequencies, has inlets for phase sync and phase modulation.
