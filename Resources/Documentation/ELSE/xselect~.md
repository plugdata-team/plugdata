---
title: xselect~

description: select input with crossfade

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

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

methods:
  - type: time <float>
    description: crossfade time in ms

draft: false
---

[xselect~] selects between multiple inputs with equal power crossfading.
