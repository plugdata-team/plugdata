---
title: mpe.in

description: get MIDI polyphonic expression data

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: port number to listen to
  default: 0 (all)

inlets:
  1st:
  - type: float
    description: raw MIDI data
  2nd:
  - type: float
    description: set port number to listen to (0 means all ports)

outlets:
  1st:
  - type: list
    description: voice number, type and data
  end:
  - type: float
    description: port number

methods:
  - type: ext <float>
    description: only listen to external input source if nonzero

flags:
- name: -ext
  description: only listen to external input source
  default: log

draft: false
---
[mpe.in] sends "cooked" MPE data from external "raw" MIDI data or an internally connected device. It outputs most common MIDI messages prepended with a voice number indexed from 0 (which depends on MIDI channel in this case).
