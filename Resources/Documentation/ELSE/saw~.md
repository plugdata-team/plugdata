---
title: saw~

description: sawtooth oscillator

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
    description: sets frequency input in MIDI pitch
  - name: -mc <list>
    description: sets multichannel output with a list of frequencies
  - name: -soft
    description: sets to soft sync mode

methods:
  - type: midi <float>
    description: nonzero sets frequency input in MIDI pitch
  - type: -set <float, float>
    description: <channel, freq> set a single frequency channel
  - type: -soft <float>
    description: nonzero sets to soft sync mode

inlets:
  1st:
  - type: signals
    description: frequency in Hz
  - type: float
    description: frequency in MIDI
  2nd:
  - type: signal
    description: phase sync (resets internal phase)
  3rd:
  - type: signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signals
    description: sawtooth wave signal

draft: false
---

[saw~] is a sawtooth oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation.
