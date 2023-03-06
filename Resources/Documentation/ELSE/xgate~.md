---
title: xgate~

description: route an input with crossfade 

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
  - type: float
    description: number of outlets/channels (min 1, max 500)
    default: 1
  - type: float
    description:  crossfade time in ms 
    default: 0
  - type: float
    description: initially selected channel
    default: 0

  
inlets:
1st:
  - type: signal
    description: input signal to route
  - type: float
    description: selected outlet (0 is none)
    
outlets:
  nth:
  - type: signal
    description: routed output
  2nd:
  - type: list
    description: reports output number and off status

methods:
  - type: time <float>
    description: crossfade time in ms

draft: false
---

[xgate~] routes between multiple outputs with equal power crossfading.
