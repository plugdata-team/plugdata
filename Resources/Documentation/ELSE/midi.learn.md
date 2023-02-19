---
title: midi.learn

description: MIDI learn

categories:
- object

pdcategory: ELSE, MIDI

arguments:
- type: symbol
  description: send name
  default: none

inlets:
  1st:
  - type: bang
    description: activate MIDI learn

outlets:
  1st:
  - type: list
    description: MIDI from learned input
  2nd:
  - type: anything
    description: learned controller

methods:
  - type: query
    description: print send stored input on right outlet
  - type: forget
    description: forget input
  - type: set <symbol>
    description: set send name
  - type: teach <anything>
    description: teach a specific MIDI message

draft: false
---

[midi.learn] is an abstraction based on [savestate] that learns and saves any MIDI input data. Activate it via bang or click and send it MIDI data so it learns where it comes from. The information gets stored in the owning patch.
The Learned control/program change are followed by numbers that specify 'control/program number' and 'channel' or just a 'channel number' for note and bend messages (touch and polytouch are not supported).
