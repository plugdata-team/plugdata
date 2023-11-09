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

outlets:
  1st:
  - type: float
    description: raw MIDI stream

flags:
- name: -rel
  description: sets the object to output release velocity and note on/off flag
- name: -both
  description: sets the object to output both note on and off velocity

draft: false
---

[note.out] formats and sends raw MIDI pitch messages to Pd's MIDI output and its outlet. An argument sets channel number (the default is 1).