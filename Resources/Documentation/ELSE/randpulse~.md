---
title: randpulse~

description: Random pulse train oscillator

categories:
 - object

pdcategory: Noise

arguments:
- type: float
  description: frequency in hz
  default: 0
- type: float
  description: initial pulse width
  default: 0.5

inlets:
  1st:
  - type: float/signal
    description: frequency in hz
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
    description: - non-zero sets to random gate value mode

draft: false
---

[randpulse~] is a random pulse train oscillator (which alternates between a random value and 0, or on/off). It accepts negative frequencies, has inlets for pulse width and sync.