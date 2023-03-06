---
title: makenote2

description: [makenote] variation

categories:
- object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: BPM
  default: 120

inlets:
  1st:
  - type: list
    description: with at least pitch, velocity and duration in beats
  2nd:
  - type: float
    description: DPM for duration

outlets:
  1st:
  - type: list
    description: note on and off messages



draft: false
---

Similarly to [makenote], [makenote2] takes a list with pitch, velocity and duration. Extra information is possible in this list, like a float for MIDI channel or whatever, but a list with at least 3 elements is necessary. The duration is in beats and the BPM is set via the argument or right inlet. The first item on the list (pitch) can be a symbol (like 'C4'). If you want the duration to specify a value in ms, use '60.000' as the bpm value!
