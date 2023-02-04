---
title: bend.out

description: MIDI pitch bend output

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: sets channel number
  default: 1

flags:
- name: -raw
  description: sets to raw input mode (0-16383)

inlets:
  1st:
  - type: float
    description: pitch bend values
  2nd:
  - type: float
    description: MIDI channel

outlets:
  1st:
  - type: float
    description: raw MIDI stream

draft: false
---

[bend.out] formats and sends raw MIDI pitch bend messages. By default, it takes normalized values (floats from -1 to 1). An argument sets channel number (the default is 1).
