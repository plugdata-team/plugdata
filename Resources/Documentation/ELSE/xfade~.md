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
    description: left 'n' inlets are channels of input A. middle 'n' inlets are channels of input B
  2nd: #rightmost
  - type: float/signal
    description: mix value from -1 (A) to 1 (B)
    
outlets:
  nth:
  - type: signal
    description: crossfaded output(s)

draft: false
---

[xfade~] performs an equal power crossfade between two sources that can have 1 to 64 channels each.
