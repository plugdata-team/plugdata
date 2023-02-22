---
title: flush
description: flush hanging note-ons
categories:
 - object
pdcategory: cyclone, MIDI
arguments:
inlets:
  1st:
  - type: bang
    description: sends note off for hanging notes
  - type: float
    description: sets the pitch part of the note message
  - type: list
    description: sets a pitch/velocity pair of a note message
  2nd:
  - type: float
    description: sets the velocity part of the note message
outlets:
  1st:
  - type: float
    description: the pitch part of a note message
  2nd:
  - type: float
    description: the velocity part of a note message

methods:
  - type: clear
    description: clears the hanging notes that [flush] keeps track off

draft: false
---

Like a "panic button", [flush] keeps track of Note-on messages that weren't switched off and "flushes" them by sending corresponding note-offs when it receives a bang.

