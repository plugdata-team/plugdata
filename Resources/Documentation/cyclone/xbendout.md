---
title: xbendout

description: send 14-bit MIDI pitch bend messages

categories:
 - object

pdcategory: cyclone, MIDI

arguments:
- type: float
  description: initial MIDI channel
  default:

inlets:
  1st:
  - type: float
    description: 14-bit pitch bend values (0-16383)
  2nd:
  - type: float
    description: MIDI channel

outlets:
  1st:
  - type: float
    description: raw MIDI data stream

draft: true
---

[xbendout] formats and sends messages that occupy both bytes of the MIDI pitch bend message (14 bit from 0 - 16383).
