---
title: pattern

description: rhythmic pattern sequencer

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- description: sets sequence
  type: list
  default: empty

inlets:
  1st:
  - type: float
    description: non-0 starts, zero stops the sequencer
  - type: bang
    description: (re)start sequencer
  - type: list
    description: sets new sequence and (re)starts sequencer
  2nd:
  - type: list
    description: sets new sequence

outlets:
  1st:
  - type: float/bang
    description: bang or index float at every new note
  2nd:
  - type: anything
    description: several sequence information

flags:
  - name: -tempo <float>
    description: sets new tempo value (default 120)
  - name: -i
    description: sets to output index (default bang)

methods:
  - type: tempo <float>
    description: sets tempo in bpm
  - type: start
    description: starts the sequencer
  - type: stop
    description: stops the sequencer
  - type: clear
    description: clears sequence

draft: false
---

[pattern] is a rhythmic pattern sequencer. The note durations are specified as floats or fractions (where '1' is a whole note), negative values are rests! A '|' separates into different bars. When you set a new pattern via the right inlet, it only starts when the previous one ends.

