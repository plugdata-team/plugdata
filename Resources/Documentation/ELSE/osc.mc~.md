---
title: osc.mc~
description: Multichannel oscillator bank

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: list
  description: list of frequencies
  default: 0

flags:
- name: -sum
  description: sums the oscillators into a single channel

inlets:
  1st:
  - type: signals
    description: frequency values
  2nd:
  - type: signal
    description: phase sync
  3rd:
  - type: signal
    description: phase modulation

outlets:
  1st:
  - type: signals
    description: multichannel signal from oscillator bank

methods:
  - type: sum <float>
    description: non zero sums the oscillators into a single channel
  - type: set <list>
    description: set frequencies

draft: false
---

[osc.mc~] takes multichannel signals to set frequencies, phase sync and phase modulation of sine oscillators. The number of oscillators is defined by the number of channels in the frequency input signal. The output is also a multichannel signal by default, but you can “sum” them into a single channel with the “sum” flag or message.
