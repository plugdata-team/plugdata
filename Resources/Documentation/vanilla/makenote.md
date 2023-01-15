---
title: makenote
description: schedule delayed 'note off' message for a note-on
categories:
- object
pdcategory: I/O 
last_update: '0.33'
see_also:
- stripnote
arguments:
- description: initial velocity value 
  default: 0
  type: float
- description: initial duration value 
inlets:
  1st:
  - type: float
    description: MIDI pitch
  2nd:
  - type: float
    description: MIDI velocity
  3rd:
  - type: float
    description: MIDI note duratin in ms
outlets:
  1st:
  - type: float
    description: MIDI pitch
  2nd:
  - type: float
    description: MIDI velocity

methods:
  - type: clear
    description: clear memory
  - type: stop
    description: flush hanging note on messages

