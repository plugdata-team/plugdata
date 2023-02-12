---
title: suspedal

description: sustain pedal

categories:
 - object
 
pdcategory: ELSE, MIDI

arguments:
- type: float
  description: float - non-0 turns on the pedal 
  default: 0 - off

inlets:
  1st:
  - type: float
    description: the pitch part of a note message
  - type: list 
    description: a pair of pitch/velocity values from note messages
  2nd:
  - type: float
    description: the velocity part of a note message
  3rd:
  - type: float
    description: pedal switch: on <1> or off <0>
    
outlets:
  1st:
  - type: float
    description: the pitch value of a note message
  2nd:
  - type: float
    description: the velocity value of a note message

flags:
- name: -retrig
  description: sets retrigger mode <0, 1, 2 or 3> (default 0)
- name: -tonal
  description: sets sustain to "tonal" mode

methods: 
  - type: clear 
    description: clears the memory (no note-off messages are output)
  - type: flush 
    description: outputs all held note-off messages
  - type: tonal <float> 
    description: non-0 sets to "tonal" mode
  - type: retrig <float> 
    description: sets retrigger mode <0, 1, 2 or 3>
  - type: sustain <float> 
    description: pedal switch: on <1> or off <0>
    
draft: false
---

[suspedal] holds MIDI Note-Off messages when on and sends them out when it's switched off. It has two sustain modes, 'regular' (default) and 'tonal' - as in the design of modern pianos.
