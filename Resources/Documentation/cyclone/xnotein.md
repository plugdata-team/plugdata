---
title: xnotein

description: retrieve velocity messages

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
    description: MIDI note number (pitch)
  2nd:
  - type: float
    description: velocity of note on and note off messages
  3rd:
  - type: float
    description: state (0 = note off, 1 = note on)
  4th:
  - type: float
    description: MIDI channel

draft: true
---

[xnotein] is more powerful than [notein] as it retrieves (from raw MIDI data streams) not only Note On Velocity but also Note Off (Release) velocity.

