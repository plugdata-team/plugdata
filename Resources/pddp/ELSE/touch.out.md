---
title: touch.out

description: Midi aftertouch input

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: sets channel number
  default: 1

flags:
- name: -poly
  description: sets the object to output polyphonic aftertouch

inlets:
  1st:
  - type: float
    description: MIDI Aftertouch
  2nd:
  - type: float
    description: MIDI channel

outlets:
  1st:
  - type: float
    description: raw MIDI stream

draft: false
---

[touch.out] formats and sends raw MIDI aftertouch messages. An argument sets channel number (the default is 1).