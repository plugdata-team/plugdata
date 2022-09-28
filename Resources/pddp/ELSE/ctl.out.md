---
title: ctl.out

description: MIDI control output

categories:
 - object

pdcategory: MIDI

arguments:
- type: list
  description: float sets channel number. Two floats set control number and channel
  default: 1

inlets:
  1st:
  - type: float
    description: control value
  2nd:
  - type: float
    description: control number
  3rd:
  - type: float
    description: MIDI channel

outlets:
  1st:
  - type: float
    description: raw MIDI stream

draft: false
---

[ctl.out] formats and sends raw MIDI control messages. An argument sets channel number (the default is 1).