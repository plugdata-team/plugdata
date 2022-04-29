---
title: stripnote
description: strip 'note off' messages
categories:
- object
pdcategory: I/O via MIDI, OSC, and FUDI
last_update: '0.28'
see_also:
- makenote
inlets:
  1st:
  - type: float
    description: MIDI pitch.
  2nd:
  - type: float
    description: MIDI velocity (no output if equal to zero).
outlets:
  1st:
  - type: float
    description: MIDI pitch.
  2nd:
  - type: float
    description: MIDI velocity.
draft: false
---
Stripnote ignores note-off (zero-velocity) messages from a stream of MIDI-style note message and passes the others through unchanged. It can deal with any kind of number (negative,  floats,  whatever) even though MIDI values need to be integers from 0 to 127!

The left inlet takes the note number and the right inlet takes velocity values. Alternatively,  you can send it a list that spreads the values through the inlets.

This is very useful if you want a Note-On message to trigger something in Pd but you don't want a Note-Off to trigger anything when you release the note.
