---
title: bpm

description: Convert to/from bpm

categories:
- object

pdcategory: Math

arguments: none

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
