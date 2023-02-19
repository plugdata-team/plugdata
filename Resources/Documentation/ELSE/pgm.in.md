---
title: pgm.in

description: MIDI program input

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
    description: MIDI Program
  2nd:
  - type: float
    description: MIDI channel

draft: false
---

[pgm.in] extracts MIDI Program information from raw MIDI input (such as from [midiin]). Unlike vanilla's [pgmin] and [pgmout], the program change values are from 0 to 127!

