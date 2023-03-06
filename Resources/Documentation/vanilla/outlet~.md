---
title: outlet~
description: audio outlet
categories:
- object
see_also: 
- outlet
- inlet
- inlet~
- block~
- pd
pdcategory: vanilla, UI, Mixing and Routing, Audio I/O
last_update: '0.51'
inlets:
  1st:
  - type: signal
    description: signal to send to parent patch
arguments:
- type: symbol
  description: downsampling method. 'hold' for sample/hold, 'pad' for zero-padded and 'lin' for linear interpolation
  default: hold
draft: false
---
