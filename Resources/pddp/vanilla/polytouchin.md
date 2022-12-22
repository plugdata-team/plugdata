---
title: polytouchin
description: MIDI input
categories:
- object
pdcategory: I/O 
last_update: 0.48-2
see_also:
- notein
- noteout
arguments:
- description: channel/port
  type: float
outlets:
  1st:
  - type: float
    description: MIDI aftertouch value.
  2nd:
  - type: float
    description: MIDI note number
  3rd:
  - type: float
    description: channel/port
  'n: (number depends on number of arguments)':
bref: MIDI input
draft: false
---

