---
title: bendin
description: MIDI input
categories:
- object
pdcategory: I/O 
last_update: 0.48-2
see_also:
- notein
- noteout
arguments:
- description: MIDI channel/port
  type: float
outlets:
  1st:
  - type: float
    description: MIDI bend value.
  2nd:
  - type: float
    description: MIDI channel/port
  'n: (number depends on number of arguments)':
bref: MIDI input
draft: false
---
**Known bug:** [bendin] and [bendout] are inconsistent ([bendin] outputs values from 0 to 16383 and [bendout] takes values from -8192 to 8191) - this won't change.
