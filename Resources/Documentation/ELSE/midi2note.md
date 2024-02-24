---
title: midi2note

description: Convert MIDI pitch to note name

categories:
- object

pdcategory: ELSE, Converters, MIDI

arguments:

inlets:
  1st:
  - type: float/list
    description: MIDI pitch value(s)
  2nd:
  - type: anything/list
    description: user defined scales

outlets:
  1st:
  - type: symbol/list
    description: note name(s)

flags:
  - name: -unicode
    description: sets to unicode mode
  - name: -list
    description: sets to list output mode

methods:
  - type: chromatic
    description: sets to chromatic mode 
    default: chromatic
  - type: sharp
    description: sets to sharp mode
  - type: flat
    description: sets to flat mode

draft: false
---

[midi2note] converts a MIDI pitch value to note names (such as Eb3). The names end with an octave number, in a way that middle C (MIDI pitch = 60) represents C4 represents. Float inputs are rounded to integers.

