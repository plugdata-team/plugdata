---
title: pgmout
description: MIDI output
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
inlets:
  1st:
  - type: float
    description: MIDI program value.
  2nd:
  - type: float
    description: MIDI channel/port
bref: MIDI output
draft: false
---
**Known bug:** Program change values in [pgmin] and [pgmout] are indexed from 1, which means that the possible values are from 1 to 128 (not 0 to 127)!
