---
title: touchin
description: MIDI input
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
outlets:
  1st:
  - type: float
    description: MIDI aftertouch value.
  2nd:
  - type: float
    description: MIDI channel/port
  'n: (number depends on number of arguments)':
bref: MIDI input
draft: false
---

