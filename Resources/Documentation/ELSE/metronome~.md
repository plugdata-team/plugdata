---
title: metronome~

description: metronome clicks

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: list
  description: low, mid, high frequencies
  default: 600, 1000, 1700

inlets:
  1st:
  - type: list
    description: count output from [metronome]

outlets:
  1st:
  - type: signal
    description: metronome clicks

flags:
 - name: -mode <float>
   description: sets mode (0, 1, 2)

methods:
  - type: set <list>
    description: sets click frequencies (low, mid, high)

draft: false
---

[metronome~] works in conjunction with [metronome] and outputs metronome clicks.
