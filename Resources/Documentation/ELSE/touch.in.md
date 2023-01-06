---
title: touch.in

description: Midi aftertouch input

categories:
 - object

pdcategory: General

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
    description: MIDI Aftertouch
  2nd:
  - type: float
    description: MIDI channel

draft: false
---

[touch.in] extracts MIDI Aftertouch information from raw MIDI input (such as from [midiin]).