---
title: hv.compressor

description: mono compressor

categories:
- object

pdcategory: heavylib, Effects, Mixing and Routing

arguments:
- type: float
  description: threshold
  default: 
- type: float
  description: ratio
  default: 

inlets:
  1st:
  - type: signal
    description: input signal
  2nd:
  - type: threshold <float>
    description: set threshold
  - type: ratio <float>
    description: set ratio

outlets:
  1st:
  - type: signal
    description: compressed signal

draft: false
---
mono compressor, static 40ms attack/release, adjustable threshold and ratio settings

