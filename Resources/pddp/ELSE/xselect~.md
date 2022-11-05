---
title: xselect~

description: Select input with crossfade

categories:
 - object
 
pdcategory: Control(Fader/Panning/Routing), General Audio Manipulation

arguments:
  - type: float
    description: number of inputs
    default: 1
  - type: float
    description: crossfade time in ms 
    default: 0
  - type: float
    description: initially selected channel
    default: 0
  
inlets:
  1st:
  - type: float
    description: selected input (0 is none)
  - type: time <f>
    description: crossfade time in ms
  2nd:
  - type: signal
    description: inputs to select from 

outlets:
  1st:
  - type: signal
    description: selected inlet/channel
  2nd:
  - type: list
    description: reports input number and off status

draft: false
---

[xselect~] selects between multiple inputs with equal power crossfading.