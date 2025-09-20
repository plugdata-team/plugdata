---
title: bl.imp2~

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
  - type: list
    description: frequencies in MIDI
  - type: signals
    description: frequencies in Hz

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

[bl.imp2~] is a two sided impulse oscillator like [else/imp2~], but it is bandlimited with the upsampling/filtering technique. This makes the object quite inefficient CPU wise, but is an easy way to implement a bandlimited oscillator.
