---
title: table~

description: Table Reader

categories:
- object

pdcategory: General

arguments:
  1st:
  - type: symbol
    description: array name (optional)
    default: none
  2nd:
  - type: float
    description: non-0 sets to guard mode
    default: 1
  3rd:
  - type: float
    description: non-0 sets to index mode
    default: 0


inlets:
  1st:
  - type: float/signal
    description: sets index/phase
  - type: set <symbol>
    description: sets an entire array to be used as a waveform
  - type: guard <float>
    description: non-0 sets to guard mode
  - type: index <float>
    description: non-0 sets to index mode


outlets:
  1st:
  - type: signal
    description: table values

draft: false
---

[table~] reads an array with 4 point interpolation. It accepts indexes from 0 to 1 by default.