---
title: xfade.mc~

description: multichannel crossfade

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
  - type: float
    description: initial mix value
    default: 0
  
inlets:
  1st:
  - type: signal
    description: multichannel input (A)
  2nd:
  - type: float/signal
    description: multichannel input (B)
  3rd:
  - type: float/signal
    description: mix value from -1 (A) to 1 (B)
    
outlets:
  1st:
  - type: signal
    description: crossfaded output

draft: false
---

[xfade.mc~] performs an equal power crossfade between two multichannel sources.