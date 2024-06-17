---
title: note2midi

description: convert note name to MIDI pitch

categories:
- object

pdcategory: ELSE, Converters, MIDI

arguments:

inlets:
  1st:
  - type: anything
    description: note name to convert to MIDI pitch

outlets:
  1st:
  - type: float
    description: MIDI pitch value

flags:
- name: -list
  description: sets to list mode

draft: false
---

[note2midi] converts note names (such as 'C4') to MIDI pitch values. All enharmonic names are included (even unusual ones like Cb and double sharps/flats). It takes symbols, lists or anythings that end with an octave number, in a way that C4 represents middle C (MIDI pitch = 60). Range is from C0 to B8.
