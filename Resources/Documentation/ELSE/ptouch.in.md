---
title: ptouch.in

description: MIDI polyphonic aftertouch input

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: sets channel number
  default: 0

inlets:
  1st:
  - description: raw MIDI data stream
    type: float
  2nd:
  - description: MIDI channel
    type: float

outlets:
  1st:
  - description: key and aftertouch
    type: list
  2nd:
  - description: MIDI channel
    type: float

methods:
- type: -ext <float>
  description: non zero sets to listen only to external input source
  
flags:
- name: -ext
  description: only listen to external input source
  default: no

draft: false
---

[ptouch.in] extracts MIDI polyphonic aftertouch information from raw MIDI input (such as from [midiin]).