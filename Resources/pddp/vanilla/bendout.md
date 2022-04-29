---
title: bendout
description: MIDI output
categories:
- object
pdcategory: I/O via MIDI, OSC, and FUDI
last_update: 0.48-2
see_also:
- notein
- noteout
arguments:
- description: MIDI channel/port
  type: float
inlets:
  1st:
  - type: float
    description: MIDI bend value.
  2nd:
  - type: float
    description: MIDI channel/port
bref: MIDI output
draft: false
---
**Known bug:** [bendin] and [bendout] are inconsistent ([bendin] outputs values from 0 to 16383 and [bendout] takes values from -8192 to 8191) - this won't change.
