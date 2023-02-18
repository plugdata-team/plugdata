---
title: poly
description: MIDI-style polyphonic voice allocator
categories:
- object
pdcategory: vanilla, MIDI, Triggers and Clocks
last_update: '0.25'
see_also:
- makenote
- route
arguments:
- description: number of voices 
  default: 1
  type: float
- description: non-0 sets to voice stealing
  type: float
inlets:
  1st:
  - type: float
    description: MIDI pitch value
  2nd:
  - type: float
    description: set velocity value
outlets:
  1st:
  - type: float
    description: the voice number
  2nd:
  - type: float
    description: note pitch
  3rd:
  - type: float
    description: note velocity
methods:
  - type: clear
    description: clear memory
  - type: stop
    description: flush hanging note on messages
draft: false
---
The poly object takes a stream of pitch/velocity pairs and outputs triples containing voice number, pitch and velocity. You can pack the output and use the route object to route messages among a bank of voices depending on the first outlet. Another option is to connect it [clone] so you can route to different copies. Poly can be configured to do voice stealing or not (the default.
