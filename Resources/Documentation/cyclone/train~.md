---
title: train~

description: pure-train generator and metronome

categories:
 - object

pdcategory: cyclone, Triggers and Clocks

arguments:
- type: float
  description: period in ms
  default: 1000
- type: float
  description: pulse width
  default: 0.5
- type: float
  description: onset phase
  default: 0

inlets:
  1st:
  - type: signal
    description: period in ms
  2nd:
  - type: signal
    description: pulse width
  3rd:
  - type: signal
    description: onset phase

outlets:
  1st:
  - type: signal
    description: pure-train signal
  2nd:
  - type: bang
    description: when transition from 0 to 1 occurs

draft: true
---

[train~] generates a pulse signal alternating from on (1) to off (0) at a period given in ms. It also sends a bang when going from 0 to 1, so it can be used as a metronome.
A pulse width of 0 has the smallest "on" pulse size (a single sample), while a pulse width of 1 has the largest (the entire period except the last sample). The "onset phase" delays the "on" portion by a fraction of the total pulse period.
