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
pdcategory: Subwindows
last_update: '0.51'
inlets:
  1st:
  - type: signal
    description: signal to send to parent patch.
arguments:
- type: symbol
  description: "downsampling method: 'hold' for sample/hold (default), 'pad' for zero-padded and 'lin' for linear interpolation)."

draft: false
---
Inlets/outlets are used to receive and get information on a patch window. This can be an abstraction or a subpatch. 