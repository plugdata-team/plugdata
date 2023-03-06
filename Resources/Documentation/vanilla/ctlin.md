---
title: ctlin
description: MIDI input
categories:
- object
pdcategory: vanilla, MIDI
last_update: 0.48-2
see_also:
- notein
- noteout
arguments:
- description: MIDI controller number
  type: float
- description: MIDI channel/port
  type: float
outlets:
  1st:
  - type: float
    description: MIDI controller value
  2nd:
  - type: float
    description: MIDI controller number
  3rd:
  - type: float
    description: MIDI channel/port
draft: false
---

