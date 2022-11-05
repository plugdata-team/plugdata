---
title: suspedal

description: Sustain pedal

categories:
 - object
 
pdcategory: General

arguments:
- type: float
  description: float - non-zero turns on the pedal 
  default: 0 - off

flags:
- name: -retrig
  description: sets retrigger mode <0, 1, 2 or 3> (default 0)
- name: -tonal
  description: sets sustain to "tonal" mode
	
  
inlets:
  1st:
  - type: float
    description: the pitch part of a note message
  - type: list 
    description: a pair of pitch/velocity values from note messages
  - type: clear 
    description: clears the memory (no note-off messages are output)
  - type: flush 
    description: outputs all held note-off messages
  - type: tonal <float> 
    description: non zero sets to "tonal" mode
  - type: retrig <float> 
    description: sets retrigger mode <0, 1, 2 or 3>
  - type: sustain <float> 
    description: pedal switch: on <1> or off <0>
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

draft: false
---

[suspedal] holds MIDI Note-Off messages when on and sends them out when it's switched off. It has two sustain modes, 'regular' (default) and 'tonal' - as in the design of modern pianos.
