---
title: bpm

description: convert to/from bpm

categories:
- object

pdcategory: ELSE, Data Math, Triggers and Clocks

arguments:
inlets:
  1st:
  - type: float
    description: input to convert

outlets:
  1st:
  - type: float
    description: converted output

flags:
- name: -samps
  description: sets conversion to/from samples

draft: false
---

[bpm] calculates a conversion to and from bpm (beats per minute). By default it converts to and from ms, but you can also set it to convert to and from samples.
