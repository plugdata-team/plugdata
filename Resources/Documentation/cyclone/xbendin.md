---
title: xbendin

description: retrieve 14-bit pitch bend messages

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
    description: 14-bit pitch bend values (0-16383)
  2nd:
  - type: float
    description: MIDI channel

draft: true
---

[xbendin] retrieves 14-bit pitch bend messages from raw MIDI input data.
