---
title: randpulse

description: random pulses

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
- type: float
  description: initial phase
  default: 0

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
  - name: -rate <float>
    description: sets refresh rate in ms
  - name: -seed <float>
    description: sets seed

methods:
  - type: rate <float>
    description: sets refresh rate in ms
  - type: seed <float>
    description: non-0 sets to random gate value mode

draft: false
---

[randpulse] is a random pulse oscillator (which alternates between a random value and 0, or on/off) at control rate with a default resolution of 1 ms). It accepts negative frequencies, has inlets for pulse width and sync.
