---
title: tri~

description: triangular wave oscillator

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
  - type: list/signals
    description: frequencies as MIDI or Hz
  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)

outlets:
  1st:
  - type: signals
    description: triangular wave signal

draft: false
---

[tri~] is a triangular wave oscillator that accepts negative frequencies, has inlets for phase sync and phase modulation.
