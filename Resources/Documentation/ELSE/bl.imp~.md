---
title: bl.imp~

description: bandlimited two-sided impulse oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - type: float
    description: frequency in Hz
    default: 0

flags:
  - name: -midi
    description: sets frequency input in MIDI pitch
    default: Hz
  - name: -mc <list>
    description: sets multichannel output with a list of frequencies

inlets:
  1st:
  - type: list/signals
    description: frequency in Hz

outlets:
  1st:
  - type: signals
    description: two sided impulse signal

methods:
  - type: midi <float>
    description: non-0 sets to frequency input in MIDI pitch
  - type: set <float, float>
    description: <channel, freq> sets a single frequency channel

draft: false
---

[bl.imp~] is an impulse oscillator like [else/imp~], but it is bandlimited.
