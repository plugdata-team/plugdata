---
title: randpulse~

description: random pulse train oscillator

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: initial pulse width
  default: 0.5

inlets:
  1st:
  - type: list/signals
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: pulse width (0-1)
  3rd:
  - type: float/signal
    description: phase sync (internal phase reset)

outlets:
  1st:
  - type: signals
    description: random pulse signal

flags:
  - name: -seed <float>
    description: seed value
  - name: -ch <float>
    description: number of output channels
    default: 1
  - name: -mc <list>
    description: sets multichannel output with a list of frequencies

methods:
  - type: seed <float>
    description: non-0 sets to random gate value mode
  - type: set <float, float>
    description: <channel, freq> set a single density channel

draft: false
---

[randpulse~] is a random pulse train oscillator (which alternates between a random value and 0, or on/off). It accepts negative frequencies, has inlets for pulse width and sync.
