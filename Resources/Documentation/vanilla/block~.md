---
title: block~
description: set block size for DSP
categories:
- object
see_also: 
- fft~
- bang~
- switch~
pdcategory: vanilla, Analysis, Audio I/O
last_update: '0.43'
inlets:
  1st:
  - type: set <list>
    description: set argument values (size, overlap, up/downsampling)
arguments:
- type: float
  description: set block size 
  default: 64
- type: float
  description: set overlap for FFT 
  default: 1
- type: float
  description: up/down-sampling factor
  default: 1
draft: false
---
set block size and on/off control for DSP
