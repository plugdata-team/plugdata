---
title: mono~

description: monophonic signal voice manager

categories:
 - object
pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: glide time in ms
  default: 0
- type: float
  description: non zero sets to legato mode
  default: 0

inlets:
  1st:
  - description: MIDI note pair (note and velocity)
    type: list
  - description: MIDI note
    type: float
2nd:
  - description: MIDI velocity
    type: float

outlets:
  1st:
  - description: MIDI pitch values
    type: signal
  2nd:
  - description: velocity values, normalized from 0-1
    type: signal
  3rd:
  - description: impulses for retriggering envelopes
    type: signal

flags:
  - name: -last
    description: select last note
  - name: -high
    description: select highest note
  - name: -low
    description: select lowest note

methods:
  - type: glide <float>
    description: set glide time in ms
  - type: mode <float>
    description: sets priority mode, 0=last, 1=high, 2=low
  - type: legato <float>
    description: non zero sets to legato mode, zero restores default
  - type: flush
    description: sends a note off for the hanging note and clears memory
  - type: clear
    description: clear memory without output
  
draft: false
---
[mono~] emulates monophonic synth behaviour and is much like [mono], but can also glide the pitch output with a portamento setting.
