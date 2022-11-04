---
title: voices

description: Polyphonic voice allocator

categories:
 - object
 
pdcategory: General

arguments:
  1st:
  - type: float
    description: sets number of voices 
    default: 1
  2nd:
  - type: float
    description: non zero sets voice stealing
    default: 0

flags:
- name: -rel <float>
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
 - type: rel <float>
    description: sets a release time in ms
 - type: offset <float>
    description: sets index offset (in the context of "list" mode)
 - type: retrig <float>
    description: non zero sets to retrigger mode
 - type: clear
    description: clears memory without output
 - type: flush
    description: clears memory and output hanging notes
 - type: voices <float>
    description: sets number of voices (in list mode only)
  2nd:
  - type: float
    description: note velocity values
  3rd:
  - type: float
    description: release time in ms
    
outlets:
  Nth:
  - type: list
    description: note messages

draft: false
---

[voices] is used to implement polyphonic synths. It's like Pd Vanilla's [poly] object, but with more functionalities. By default, it routes the voices to different outlets, but it also has a list mode similar to [poly].
