---
title: hilbert~
description: Hilbert transform
categories:
- object
see_also:
- complex-mod~
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
last_update: '0.52'
inlets:
  1st:
  - type: signal
    description: signal input.
  2nd:
  - type: bang 
    description: clear filter's state.	
outlets:
  1st:
  - type: signal
    description: real part of transformed signal.
  2nd:
  - type: signal
    description: imaginary part of transformed signal.
draft: false
---
The Hilbert transform (the name is abused here according to computer music tradition) puts out a phase quadrature version of the input signal suitable for signal sideband modulation via complex-mod~.