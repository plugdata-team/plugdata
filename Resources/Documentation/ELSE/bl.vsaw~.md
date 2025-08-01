---
title: bl.vsaw~

description: bandlimited sawtooth-triangular oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: frequency in Hz
    type: float
    default: 0
  - description: initial width
    type: float
    default: 0
  - description: initial phase offset
    type: float
    default: 0

flags:
  - name: -midi
    description: sets frequency input in MIDI pitch
    default: Hz
  - name: -soft
    description: sets to soft sync mode
    default: hard
  - name: -mc <list>
    description: sets multichannel output with a list of frequencies

inlets:
  1st:
  - type: list/signals
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: pulse width (from 0 to 1)
  3rd:
  - type: float/signal
    description: phase sync (resets internal phase)
  4th:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signals
    description: variable sawtooth-triangle wave signal

methods:
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: soft <float>
    description: non-0 sets to soft sync mode
  - type: set <float, float>
    description: <channel, freq> sets a single frequency channel

draft: false
---

[bl.vsaw~] is a variable sawtooth waveform oscillator that also becomes a triangular osccilator just like [else/vsaw~], but it is bandlimited.
