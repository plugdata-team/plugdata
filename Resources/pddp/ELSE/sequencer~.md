---
title: sequencer~

description: Signal sequencer

categories:
 - object

pdcategory: Control (Sequencers)

arguments:
- type: list <f>
  description: list of floats sets the sequence 
  default: 0

inlets:
- type: signal
  description: signal trigger to generate value from sequence
- type: bang
  description: control trigger to generate value from sequence
- type: goto <f>
  description: goes to a position index in the sequence (from 1)
- type: set <list>
  description: set a new sequence

outlets:
  1st:
  - type: signal
    description: value from a sequence
  2nd:
  - type: signal
    description: impulse when reaching end of sequence

draft: false
---

When receiving a signal trigger (0 to non-0 transition) or a bang, [sequencer~] sends a signal value from a given sequence.
