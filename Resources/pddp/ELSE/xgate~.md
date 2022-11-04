---
title: xgate~

description: Route an input with crossfade 

categories:
 - object
 
pdcategory: General

arguments:
  1st:
  - type: float
    description: number of outlets/channels (min 1, max 500)
    default: 1
  2nd:
  - type: float
    description:  crossfade time in ms 
    default: 0
  3rd:
  - type: float
    description: initially selected channel
    default: 0

  
inlets:
1st:
  - type: signal
    description: input signal to route
  - type: float
    description: selected outlet (0 is none)
  - type: time <float>
    description: crossfade time in ms
    
outlets:
  Nth:
  - type: signal
    description: routed output
  Nth+1:
  - type: list
    description: reports output number and off status

draft: false
---

[xgate~] routes between multiple outputs with equal power crossfading.
