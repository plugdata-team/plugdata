---
title: sequencer~

description: signal sequencer

categories:
 - object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: list <f>
  description: list of floats sets the sequence 
  default: 0

inlets:
  1st:
  - type: signal
    description: signal trigger to generate value from sequence
  - type: bang
    description: control trigger to generate value from sequence

outlets:
  1st:
  - type: signal
    description: value from a sequence
  2nd:
  - type: signal
    description: impulse when reaching end of sequence


methods:
- type: goto <float>
  description: goes to a position index in the sequence (from 1)
- type: set <list>
  description: set a new sequence

draft: false
---

When receiving a signal trigger (0 to non-0 transition) or a bang, [sequencer~] sends a signal value from a given sequence.
