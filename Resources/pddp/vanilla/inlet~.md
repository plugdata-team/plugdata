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
pdcategory: Subwindows
last_update: '0.51'
inlets:
  1st:
  - type: float
    description: set default value for no signal connected.
outlets:
  1st:
  - type: signal
    description: signal from parent patch.
  2nd:
  - type: anything
    description: any message from parent patch when give a 'fwd' argument.
arguments:
- type: symbol
  description: "'fwd' to turn message forwarding on. Alternatively, you can set upsampling method: 'hold' for sample/hold (default), 'pad' for zero-padded and 'lin' for linear interpolation)."

draft: false
---
Inlets/outlets are used to receive and get information on a patch window. This can be an abstraction or a subpatch. 