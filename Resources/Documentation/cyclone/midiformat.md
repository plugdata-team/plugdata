---
title: midiformat
description: format MIDI messages
categories:
 - object
pdcategory: cyclone, MIDI
arguments:
- type: float
  description: channel number
  default: 1
inlets:
  1st:
  - type: list
    description: <pitch/velocity> pairs for note on/off messages
  2nd:
  - type: list
    description: <aftertouch/pitch> pairs for polyphonic aftertouch messages
  3rd:
  - type: list
    description: <value/controller number> pairs for control messages
  4th:
  - type: float
    description: program change value for program change message
  5th:
  - type: float
    description: channel aftertouch value for channel aftertouch message
  6th:
  - type: float
    description: pitch bend value for pitch bend message
  7th:
  - type: float
    description: channel number (wraps at 16) - doesn't output MIDI messages
outlets:
  1st:
  - type: float
    description: raw MIDI messages

flags:
  - name: @hires <0, 1, 2>
    description: pitch bend precision (see help)

methods:
  - type: hires <f>
    description: sets 14-bit pitch bend input precision (see help)

draft: false
---

[midiformat] receives different MIDI information in each inlet and formats to a raw MIDI message output.

