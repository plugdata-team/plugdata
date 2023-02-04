---
title: kink~
description: phase distortion
categories:
 - object
pdcategory: cyclone, Effects
arguments:
- type: float
  description: slope (minimum 0)
  default: 1
inlets:
  1st:
  - type: signal
    description: phasor signal to be distorted by kinking it
  2nd:
  - type: float/signal
    description: slope (minimum 0)
outlets:
  1st:
  - type: signal
    description: distorted phasor output

---

Distort [phasor~] with [kink~]. If the phase input times the slope is less than 0.5, the value is output. Otherwise, a complentary slope is used that goes from 0.5 to 1 (creating a bend or a "kink").

