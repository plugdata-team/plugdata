---
title: freq2midi

description: convert frequency to MIDI

categories:
- object

pdcategory: ELSE, MIDI, Tuning, Converters

arguments:
- description: set A4 value
  type: float
  default: 440

inlets:
  1st:
  - type: list
    description: Hz value(s) to convert
  2nd:
  - type: float
    description: set a new value for A4

outlets:
  1st:
  - type: list
    description: value(s) in MIDI

draft: false
---

[freq2midi] converts frequency to MIDI like [ftom], but it also converts lists and allows you so set a different frequency reference value for A4 so you can explore different tunings.

