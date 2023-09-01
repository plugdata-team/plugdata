---
title: phaseseq~

description: impulse sequence from phasor input

categories:
 - object
 
pdcategory: ELSE, Signal Generators, Triggers and Clocks

arguments:
- type: list
  description: threshold values
  default: none

inlets:
  1st:
  - type: signal
    description: phase signal input

outlets:
  1st:
  - type: signal
    description: impulse at sequence's values
  2nd:
  - type: signal
    description: index from sequence

methods:
  - type: set <list>
    description: set threshold values

draft: false
---

[phaseseq~] takes a list of thresholds and outputs impulses when reaching the threshold values.