---
title: xfade~

description: multi-channel crossfade

categories:
 - object
 
pdcategory: ELSE, Mixing and Routing

arguments:
  - type: float
    description: number of 'n' channels for each source
    default: 1
  - type: float
    description: initial mix value
    default: 0
  
inlets:
  nth:
  - type: signal
    description: left inlets are the 'n' channels of input A
    middle inlets are the 'n' channels of input B
  - type: float/signal
    description: rightmost inlet is the mix value from -1 (A) to 1 (B)
    
outlets:
  1st:
  - type: signal
    description: 'n' crossfaded channels

draft: false
---

[xfade~] performs an equal power crossfade between two sources that can have 1 to 64 channels each.
