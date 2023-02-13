---
title: voices

description: polyphonic voice allocator

categories:
 - object
 
pdcategory: ELSE, Data Management, MIDI

arguments:
  - type: float
    description: sets number of voices 
    default: 1
  - type: float
    description: non-0 sets voice stealing
    default: 0

flags:
- name: -rel <f>
  description: sets release time in ms (default: 0)
- name: -retrig
  description: sets to retrigger mode <0, 1 or 2> 
  (default: 0)
- name: -list <float>
  description: sets to list mode
  
inlets:
  1st:
  - type: list
    description: MIDI note messages (note and velocity pair)
  - type: float
    description: note pitch values
  2nd:
  - type: float
    description: note velocity values
  3rd:
  - type: float
    description: release time in ms
    
outlets:
  nth:
  - type: list
    description: note messages

methods:
  - type: rel <float>
    description: sets release time in ms
  - type: offset <float>
    description: sets index offset (in the context of "list" mode)
  - type: retrig <float>
    description: non-0 sets to retrigger mode
  - type: clear
    description: clears memory without output
  - type: flush
    description: clears memory and output hanging notes
  - type: voices <float>
    description: sets number of voices (in list mode only)

draft: false
---

[voices] is used to implement polyphonic synths. It's like Pd Vanilla's [poly] object, but with more functionalities. By default, it routes the voices to different outlets, but it also has a list mode similar to [poly].
