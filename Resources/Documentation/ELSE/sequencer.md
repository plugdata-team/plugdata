---
title: sequencer

description: data sequencer

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: list
  description: list of elements sets the sequence
  default: empty

inlets:
  1st:
  - type: bang
    description: gets element(s) from sequence
  - type: float
    description: sets index (from 1) and outputs it
  2nd:
  - type: list
    description: sets a new sequence
  - type: bang
    description: sets an empty sequence

outlets:
  1st:
  - type: float
    description: element(s) from a sequence
  2nd:
  - type: bang
    description: when there is a rest
  3rd:
  - type: float
    description: bar number
  4th:
  - type: bang
    description: when reaching end of sequence

methods:
  - type: goto <f>
    description: goes to a position index in the sequence (from 1) 
  - type: set <list>
    description: set a new sequence
  - type: clear
    description: clears sequence

draft: false
---

[sequencer] sends an element from a given sequence when banged. Elements joined by "-" are sent at the same time but separately (allowing chords). A single "-" is treated as a rest. A "_" is ignored and can be thought as "tie". A "|" represents a barline. Symbol elements are sent without a symbol selector (and can be note names or anything else). A "bang" comes out as a proper "bang".
