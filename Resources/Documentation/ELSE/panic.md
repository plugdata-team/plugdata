---
title: panic

description: flush hanging MIDI notes

categories:
 - object
pdcategory: ELSE, MIDI

arguments:

inlets:
  1st:
  - type: bang
    description: sends note off for hanging notes
  - type: float
    description: raw MIDI data stream
outlets:
  1st:
  - type: float
    description: raw MIDI data stream

methods:
  - type: clear
    desctiption: clears the hanging notes that [midiflush] keeps track off

---

Like a "panic button", [panic] keeps track of raw Note-on messages that weren't switched off and "flushes" them by sending corresponding note-offs when it receives a bang.

