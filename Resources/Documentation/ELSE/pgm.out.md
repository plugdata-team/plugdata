---
title: pgm.out

description: MIDI program output

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
    description: MIDI channel

outlets:
  1st:
  - type: float
    description: raw MIDI stream

draft: false
---

[pgm.out] formats and sends raw MIDI program messages. An argument sets channel number (the default is 1). Unlike vanilla's [pgmin] and [pgmout], the program change values are from 0 to 127!

