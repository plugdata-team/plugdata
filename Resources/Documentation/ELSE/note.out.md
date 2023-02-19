---
title: note.out

description: MIDI pitch output

categories:
 - object

pdcategory: ELSE, MIDI

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
    description: note on/off flag (if -rel flag is given)
outlets:
  1st:
  - type: float
    description: rightmost inlet is MIDI channel

flags:
- name: -rel
  description: sets the object to output release velocity and note on/off flag
draft: false
---

[pitchout] formats and sends raw MIDI pitch messages. An argument sets channel number (the default is 1).
