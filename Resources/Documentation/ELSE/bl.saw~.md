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
    description: frequency in Hz or midi
  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signals
    description: sawtooth wave signal

methods:
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: soft <float>
    description: non-0 sets to soft sync mode
  - type: set <float, float>
    description: <channel, freq> sets a single frequency channel

draft: false
---

[bl.saw~] is a sawtooth oscillator like [else/saw~], but it is bandlimited.
