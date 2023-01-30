---
title: touch.in

description: MIDI aftertouch input

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: sets channel number
  default: 0 - OMNI

flags:
- name: -poly
  description: sets the object to output polyphonic aftertouch

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
    description: MIDI aftertouch
  2nd:
  - type: float
    description: MIDI channel

draft: false
---

[touch.in] extracts MIDI aftertouch information from raw MIDI input (such as from [midiin]).
