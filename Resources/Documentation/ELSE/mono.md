---
title: mono

description: monophonic voice manager

categories:
- object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: non-0 sets to legato mode
  default:

inlets:
  1st:
  - type: list
    description: MIDI note message (note, velocity)
  - type: float
    description: note values
  2nd:
  - type: float
    description: velocity values

outlets:
  1st:
  - type: float
    description: note values
  2nd:
  - type: float
    description: velocity values

flags:
  - name: -last
    description: sets mode to "last"
  - name: -high
    description: sets mode to "high"
  - name: -low
    description: sets mode to "low"

methods:
  - type: mode <float>
    description: priority mode (0: last, 1: high, 2: low)
  - type: legato <float>
    description: non-0 - legato mode, zero - restores default
  - type: flush
    description: sends a note off for the hanging note and clears memory
  - type: clear
    description: clears memory

draft: false
---

[mono] takes note messages and handles them to emulate monophonic synth behavior. Its internal memory can remember up to the last 10 input voices *and pitches need to be >= 0).
