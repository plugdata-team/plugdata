---
title: midi.out

description: send MIDI data

categories:
 - object

pdcategory: ELSE, MIDI

inlets:
  1st:
  - description: cooked MIDI data
    type: anything

outlets:
  1st:
  - description: raw MIDI data
    type: float

flags:
  - name: -font <symbol>
    description: default 'dejavu sans mono' or 'menlo' for mac

draft: false
---

[midi.in] sends MIDI data from "cooked" data, instead of "raw" data like [midiout]. It takes data type symbol, values and channel (but a note output is received as a simple numeric list). It outputs raw MIDI data but it also sends out to the connected MIDI device.