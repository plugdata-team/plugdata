---
title: adsr.m~
description: ADSR module

categories:
 - object

pdcategory: MERDA, Envelopes and LFOs, Signal Generators

arguments:

inlets:
  1st:
  - type: signal
    description: trigger in
  2nd:
  - type: signal
    description: attack
  3rd:
  - type: signal
    description: decay
  4th:
  - type: signal
    description: sustain
  5th:
  - type: signal
    description: release

outlets:
  1st:
  - type: signal
    description: envelope out
