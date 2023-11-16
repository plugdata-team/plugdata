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
  - type: list
    description: MIDI pitch and note-on velocity
  2nd:
  - type: list
    description: MIDI pitch and note-off velocity

flags:
- name: -rel
  description: sets the object to output release velocity and note on/off flag
- name: -ext
  description: only listen to external input source
- name: -both
  description: sets the object to output both note on and off velocity

methods:
- type: ext <float>
  description: nonzero sets to listen only to external input source

draft: false
---

[note.in] extracts MIDI Pitch information from external "raw" MIDI data or an internally connected device. It deals with both NoteOn and NoteOff messages (with release velocity).