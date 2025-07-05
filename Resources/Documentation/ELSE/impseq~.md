---
title: impseq~
description: impulse generator and sequencer

categories:
 - object

pdcategory: ELSE, Signal Generators, Triggers and Clocks

arguments:
- type: list
  description: list of floats sets the sequence
  default: 1

inlets:
  1st:
  - type: signal
    description: trigger to generate impulse from sequence
  - type: bang
    description: generates impulse from sequence at control rate
outlets:
  1st:
  - type: signal
    description: impulse value from a sequence
  2nd:
  - type: signal
    description: impulse when reaching end of sequence

methods:
  - type: set <list>
    description: sets one or more impulse values without outputting
  - type: goto <float>
    description: goes to a position index in the sequence (from 1)

draft: false
---

When receiving a signal trigger (0 to non-0 transition) or a bang, [impseq~] sends an impulse from a given sequence.
