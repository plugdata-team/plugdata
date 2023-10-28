---
title: midi.in

description: get cooked MIDI data

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: MIDI channel (0 is OMNI)
  default: 0

inlets:
  1st:
  - description: raw MIDI data
    type: float
  - description: MIDI channel
    type: float
    
outlets:
  1st:
  - description: Cooked MIDI data
    type: anything
  - description: MIDI channel
    type: float


draft: false
---

[midi.in] sends "cooked" MIDI data instead of "raw" data like [midiin] with MIDI data type symbol, values and channel (but a note output is sent as a simple numeric list). It can take a raw MIDI input via its inlet but it also listens to your connected MIDI device.