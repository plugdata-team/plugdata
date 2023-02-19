---
title: keymap
description: MIDI input from computer keyboard

categories:
 - object

pdcategory: ELSE, UI, MIDI

arguments:

inlets:
  1st:
  - type: float
    description: non-0 turns it on, 0 turns it off

outlets:
  1st:
  - type: list
    description: note on/off message
  2nd:
  - type: float
    description: transposition value

draft: false
---
[keymap] maps your computer keyboard to MIDI pitches and generates note
on/off messages when keys get pressed and released. It works for any
keyboard layout. It does not work when in edit mode and autorepeated
keys get filtered.

