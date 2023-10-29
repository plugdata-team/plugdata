---
title: repeat~

description: copy an input signal to multiple channels

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: minimum
  default: 0

inlets:
  1st:
  - description: sets a new text note
    type: set <anything>

outlets:
  1st:
  - description: sets a new text note
    type: set <anything>

flags:
  - name: -font <symbol>
    description: default 'dejavu sans mono' or 'menlo' for mac
  - name: -bold
    description:


methods:
  - type: set <anything>
    description: sets a new text note
  - type: append <anything>
    description: appends a message to the note

draft: false
---

[repeat~] takes an input signal of any channel size and repeats it 'n' times.
