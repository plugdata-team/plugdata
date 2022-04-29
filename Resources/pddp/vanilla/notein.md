---
title: notein
description: MIDI input
categories:
- object
pdcategory: I/O via MIDI, OSC, and FUDI
last_update: 0.48-2
see_also:
- ctlin
- noteout
arguments:
- description: MIDI channel/port
  type: float
outlets:
  1st:
  - type: float
    description: MIDI note number.
  2nd:
  - type: float
    description: MIDI velocity
  3rd:
  - type: float
    description: MIDI channel/port
  'n: (number depends on number of arguments)':
draft: false
---

