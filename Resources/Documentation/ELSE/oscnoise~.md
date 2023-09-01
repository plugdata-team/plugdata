
---
title: oscnoise~

description: noise oscillator

categories:
- object

pdcategory: ELSE, Signal Generators, Mixing and Routing

arguments:
  - description: frequency in midi pitch or hertz
    type: float
    default: 0

flags:
  - name: -midi <float>
    description: sets frequency input in MIDI pitch (default hertz)

inlets:
  1st:
  - type: float/signal
    description: frequency in hz or midi
  - type: bang
    description: update noise table
  2nd:
  - type: signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signal
    description: noise signal

methods:
  - type: midi <float>
    description: non zero sets to frequency input in MIDI pitch

draft: false
---

[oscnoise~] is an oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation. The waveform is a table filled with white noise. A bang updates the noise table.



