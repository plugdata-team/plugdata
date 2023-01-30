---
title: sysrt.in

description: MIDI system/real-time input

categories:
- object

pdcategory: ELSE, MIDI

arguments:

inlets:
  1st:
  - type: float
    description: raw MIDI data stream

outlets:
  1st:
  - type: bang
    description: bang at MIDI clock 
  2nd:
  - type: bang
    description: bang at MIDI tick 
  3rd:
  - type: bang
    description: bang at MIDI start
  4th:
  - type: bang
    description: bang at MIDI continue 
  5th:
  - type: bang
    description: bang at MIDI stop
  6th:
  - type: bang
    description: bang at MIDI active sensing
  7th:
  - type: bang
    description: bang at MIDI reset

draft: false
---

[sysrt.in] detects System and real-time MIDI messages from raw MIDI data.
