---
title: note.in

description: MIDI pitch input

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: sets channel number
  default: 0 - OMNI
inlets:
  1st:
  - type: float
    description: raw MIDI data stream
  2nd:
  - type: float
    description: MIDI channel
outlets:
  1st:
  - type: float
    description: MIDI pitch
  2nd:
  - type: float
    description: MIDI velocity
  3rd:
  - type: float
    description: Note on/off flag (if -rel flag is given)
  4th:
  - type: float
    description: Rightmost outlet is MIDI channel

flags:
- name: -rel
  description: sets the object to output release velocity and note on/off flag

draft: false
---

[note.in] extracts MIDI Pitch information from raw MIDI input (such as from [midiin]).
