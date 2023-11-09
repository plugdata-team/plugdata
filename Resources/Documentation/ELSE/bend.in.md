---
title: bend.in

description: MIDI pitch bend input

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: sets channel number
  default: 0 - OMNI

methods:
- type: -ext <float>
  description: non zero sets to listen only to external input source
  
flags:
- name: -raw
  description: sets to raw output mode (0-16383)
- name: -ext
  description: only listen to external input source
  default: no

inlets:
  1st:
  - type: float
    description: raw MIDI data stream
  2nd:
  - type: float
    description: MIDI channel

outlets:
  1st:
  - type: float
    description: MIDI pitch bend value (from -1 to 1)
  2nd:
  - type: float
    description: MIDI channel


draft: false
---

[bend.in] extracts MIDI Pitch Bend information from raw MIDI input (such as from [midiin]). By default, output values are normalized to floats from -1 to 1, but you can change it to "raw" mode (output integers from 0 to 16383).
