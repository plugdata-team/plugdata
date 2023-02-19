---
title: inlet~
description: audio inlet
categories:
- object
see_also: 
- outlet
- inlet
- outlet~
- block~
- pd
pdcategory: vanilla, UI, Mixing and Routing, Audio I/O
last_update: '0.51'
inlets:
  1st:
  - type: float
    description: set default value for no signal connected
outlets:
  1st:
  - type: signal
    description: signal from parent patch
  2nd:
  - type: anything
    description: any message from parent patch or inlet
arguments:
- type: symbol
  description: set upsampling method. 'hold' for sample/hold, 'pad' for zero-padded, and 'lin' for linear interpolation
  default: hold
draft: false
---

