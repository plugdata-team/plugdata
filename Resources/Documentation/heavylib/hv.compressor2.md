---
title: hv.compressor2

description: stereo compressor

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
    description: left channel input
  2nd:
  - type: signal
    description: right channel input
  3rd:
  - type: threshold <float>
    description: set threshold
  - type: ratio <float>
    description: set ratio

outlets:
  1st:
  - type: signal
    description: left channel output
  2nd:
  - type: signal
    description: right channel output

draft: false
---
stereo compressor, static 40ms attack/release, adjustable threshold and ratio settings

