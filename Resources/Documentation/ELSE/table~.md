---
title: table~

description: table Reader

categories:
- object

pdcategory: ELSE, Arrays and Tables, Buffers

arguments:
  - type: symbol
    description: array name (optional)
    default: none
  - type: float
    description: non-0 sets to guard mode
    default: 1
  - type: float
    description: non-0 sets to index mode
    default: 0


inlets:
  1st:
  - type: float/signal
    description: sets index/phase

outlets:
  1st:
  - type: signal
    description: table values

methods:
  - type: set <symbol>
    description: sets an entire array to be used as a waveform
  - type: guard <float>
    description: non-0 sets to guard mode
  - type: index <float>
    description: non-0 sets to index mode


draft: false
---

[table~] reads an array with 4 point interpolation. It accepts indexes from 0 to 1 by default.
