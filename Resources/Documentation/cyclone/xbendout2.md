---
title: xbendout2

description: send 14-bit MIDI pitch bend messages

categories:
 - object

pdcategory: cyclone, MIDI

arguments:
- type: float
  description: initial MIDI channel
  default:

inlets:
  1st:
  - type: float
    description: the MSB (Most Significant Byte) 7-bit value (0-127)
  2nd:
  - type: float
    description: the LSB (Least Significant Byte) 7-bit value (0-127)
  3rd:
  - type: float
    description: MIDI channel

outlets:
  1st:
  - type: float
    description: raw MIDI data stream

draft: true
---

[xbendout2] formats and sends pitch bend messages as two 7-bit messages (values from 0-127), the Most Significant Byte (MSB) and the Least Significant Byte (LSB).
