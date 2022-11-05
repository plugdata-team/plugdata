---
title: vu~

description: Amplitude detector for [vu]

categories:
 - object
 
pdcategory: Analysis

arguments:
  - type: float
    description: analyses window size in samples
    default: 1024
  - type: float
    description: hop size in samples 
    default: half the window size
  
inlets:
 1st:
  - type: float
    description: signal to analyze
  - type: set <f,f>
    description: sets window and hop size in samples
    
outlets:
  1st:
  - type: float
    description: RMS amplitude value in dBFs
  2nd:
  - type: float
    description: peak amplitude value in dBFS

draft: false
---

[vu~] combines [rms~] and [peak~] to provide dBFs output to Pd Vanilla's [vu] GUI.