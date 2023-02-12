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
  - type: float/signal
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: pulse width (0-1)
  3rd:
  - type: float/signal
    description: phase sync (internal phase reset)

outlets:
  1st:
  - type: signal
    description: random pulse signal

flags:
  - name: -seed <float>
    description: seed value

methods:
  - type: seed <float>
    description: - non-0 sets to random gate value mode

draft: false
---

[randpulse~] is a random pulse train oscillator (which alternates between a random value and 0, or on/off). It accepts negative frequencies, has inlets for pulse width and sync.
