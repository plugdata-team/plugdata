---
title: voices~

description: multichannel signal/polyphonic voice allocator

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: sets number of voices
  default: 1

inlets:
  1st:
  - description: MIDI note messages (note and velocity pair or more)
    type: list
  2nd:
  - description: set release time
    type: float

outlets:
  1st:
  - description: pitch values in MIDI
    type: signals
  2nd:
  - description: gate values normalized to 0-1 range
    type: list

methods:
  - type: voices <float>
    description: sets number of voices (in list mode only)

draft: false
---

[voices~] is an abstraction based on [voices] and generates multichannel signals that can be used to control pitch and gate with polyphony.