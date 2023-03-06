---
title: xbendin2

description: retrieve 14-bit MIDI pitch bend messages

categories:
 - object

pdcategory: cyclone, MIDI

arguments:
- type: float
  description: sets channel number
  default: no channel

inlets:
  1st:
  - type: float
    description: raw MIDI data stream

outlets:
  1st:
  - type: float
    description: the MSB (Most Significant Byte) 7-bit value (0-127)
  2nd:
  - type: float
    description: the LSB (Least Significant Byte) 7-bit value (0-127)
  3rd:
  - type: float
    description: MIDI channel

draft: true
---

[xbendin2] retrieves the Most and Least Significant Byte (7-bits values) from pitch bend messages of incoming raw MIDI data. Both can be combined to generate a 14-bit value.
