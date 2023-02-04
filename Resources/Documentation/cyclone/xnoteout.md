---
title: xnoteout

description: send release velocity messages

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

outlets:
  1st:
  - type: float
    description: raw MIDI data stream

draft: true
---

[xnoteout] is more powerful than [noteout] as it send not only Note On Velocity but also Note Off (Release) velocity.
