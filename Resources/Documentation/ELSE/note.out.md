---
title: note.out
description: Midi pitch output
categories:
 - object
pdcategory: General

arguments:
- type: float
  description: sets channel number
  default: 1

inlets:
  1st:
  - type: float
    description: pitch values
  2nd:
  - type: float
    description: velocity values
  3rd:
  - type: float
    description: note on/off flag (if -rel flagis given)
outlets:
  1st:
  - type: float
    description: rightmost inlet is MIDI channel

flags:
- name: -rel
  description: sets the object to output release velocity and note on/off flag
---

[pitchout] formats and sends raw MIDI pitch messages. An argument sets channel number (the default is 1).