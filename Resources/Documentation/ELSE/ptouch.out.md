---
title: ptouch.out

description: MIDI polyphonic aftertouch output

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: sets channel number
  default: 0

inlets:
  1st:
  - description: key value
    type: float
  2nd:
  - description: aftertouch value
    type: float
  3rd:
  - description: MIDI channel
    type: float

outlets:
  1st:
  - description: raw MIDI stream
    type: float

draft: false
---

[ptouch.out] formats and sends raw MIDI polyphonic aftertouch messages. An argument sets channel number (the default is 1).
