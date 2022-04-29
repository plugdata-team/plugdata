---
title: ctlout
description: MIDI output
categories:
- object
pdcategory: I/O via MIDI, OSC, and FUDI
last_update: 0.48-2
see_also:
- notein
- noteout
arguments:
- description: MIDI controller number
  type: float
- description: MIDI channel/port
  type: float
inlets:
  1st:
  - type: float
    description: MIDI controller value.
  2nd:
  - type: float
    description: MIDI controller number
  3rd:
  - type: float
    description: MIDI channel/port
bref: MIDI output
draft: false
---

