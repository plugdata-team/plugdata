---
title: voices

description: Polyphonic voice allocator

categories:
 - object
 
pdcategory: General

arguments:
  - type: float
    description: sets number of voices 
    default: 1
  - type: float
    description: non zero sets voice stealing
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
 - type: rel <f>
    description: sets a release time in ms
 - type: offset <f>
    description: sets index offset (in the context of "list" mode)
 - type: retrig <f>
    description: non zero sets to retrigger mode
 - type: clear
    description: clears memory without output
 - type: flush
    description: clears memory and output hanging notes
 - type: voices <f>
    description: sets number of voices (in list mode only)
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

draft: false
---

[voices] is used to implement polyphonic synths. It's like Pd Vanilla's [poly] object, but with more functionalities. By default, it routes the voices to different outlets, but it also has a list mode similar to [poly].
