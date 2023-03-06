---
title: midi2freq

description: convert MIDI to frequency

categories:
- object

pdcategory: ELSE, Converters, MIDI, Tuning

arguments:
- type: float
  description: sets A4 value
  default: 440

inlets:
  1st:
  - type: list
    description: MIDI value(s) to convert
  2nd:
  - type: float
    description: sets a new A4 value

outlets:
  1st:
  - type: list
    description: value(s) in Hz

draft: false
---

[midi2freq] converts MIDI to frequency like [mtof], but it also converts lists and allows you so set a different frequency reference value for A4 so you can explore different tunings. Note that 'MIDI cents' are possible, where 69.5 is 50 cents (or a quarter tone) higher than 69 (hence 1 cent = 0.01).
