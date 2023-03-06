---
title: sustain

description: sustain pedal emulator

categories:
 - object

pdcategory: cyclone, MIDI

arguments: (none)

inlets:
  1st:
  - type: float
    description: pitch
  - type: list 
    description: a pair of pitch/velocity values from note message
  2nd:
  - type: float
    description: velocity
  3rd:
  - type: float
    description: sustain switch: on <1>, off <0>

outlets: 
  1st:
  - type: float
    description: the pitch value of a pitch-velocity pair
  2nd:
  - type: float
    description: the velocity value of a pitch-velocity pair

methods: 
  - type: clear
    description: clears the memory (no note-off messages are output)
  - type: flush
    description: outputs all held note-off messages
  - type: sustain <float>
    description: sustain switch: on <1>, off <0>
  - type: repeatmode <float>
    description: historical <0>, re-trigger <1> & stop last <2>

draft: true
---

[sustain] holds the note off messages while it is on and sends the held note off messages when it's switched off.
