---
title: midiparse
description: retrieve MIDI messages
categories:
 - object
pdcategory: cyclone, MIDI
arguments:
inlets:
  1st:
  - type: float
    description: raw MIDI data stream
  - type: bang
    description: clears object's memory of received MIDI data
outlets:
  1st:
  - type: list
    description: <pitch/velocity> pairs from note messages
  2nd:
  - type: list
    description: <aftertouch/pitch> pairs from polyphonic aftertouch messages
  3rd:
  - type: list
    description: <value/controller number> pairs from control messages
  4th:
  - type: float
    description: value from program change messages
  5th:
  - type: float
    description: value from channel aftertouch messages
  6th:
  - type: float
    description: value from pitch bend messages
  7th:
  - type: float
    description: the MIDI channel number from raw MIDI message

flags:
  - name: @hires <0, 1, 2>
    description: pitch bend precision (see help)

methods:
  - type: hires <f>
    description: sets 14-bit pitch bend input precision (see help)

draft: false
---

[midiparse] receives raw MIDI data and retrieves different MIDI messages from it.

