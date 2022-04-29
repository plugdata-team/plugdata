---
title: switch~
description: set block size and on/off control for DSP
categories:
- object
see_also: 
- fft~
- bang~
- block~
pdcategory: Subwindows
last_update: '0.43'
inlets:
  1st:
  - type: float
    description: nonzero turns DSP on, zero turns DSP off.
  - type: bang
    description: when turned off, computes just one DSP cycle.
  - type: set <list>
    description: set argument values (size, overlap, up/downsampling).
arguments:
- type: float
  description: set block size 
  default: 64
- type: float
  description: set overlap for FFT 