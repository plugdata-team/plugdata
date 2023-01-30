---
title: sysrt.out

description: MIDI system/real-time output

categories:
- object

pdcategory: ELSE, MIDI

arguments:

inlets:
  1st:
  - type: anything
    description: clock, tick, start, continue, sensing, reset

outlets:
  1st:
  - type: midi
    description: raw MIDI message

draft: false
---

[sysrt.out] formats and sends raw MIDI system and real-time messages.
