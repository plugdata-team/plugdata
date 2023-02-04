---
title: phaser~

description: phaser effect

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: number of stages from 2 to 64
  type: float
  default: 2
- description: q- resonance
  type: float
  default: 1
- description: dry/wet ratio from 0-1
  type: float
  default: 1

inlets:
  1st:
  - type: signal
    description: input to phaser
  2nd:
  - type: signal
    description: frequency

outlets:
  1st:
  - type: signal
    description: phaser output

methods:
  - type: q <float>
    description: internal allpass resonance
  - type: wet <float>
    description: dry/wet ratio from 0-1

draft: false
---

[phaser~] is a mono phaser effect abstraction. You can set the number of stages with the first argument, which sets the order of the internal allpass filter (needs to be a multiple of 2). It requires a signal LFO to control the frequency sweep.

