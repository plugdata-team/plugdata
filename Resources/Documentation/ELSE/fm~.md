---
title: fm~

description: frequency modulation unit

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: carrier frequency in hz
  default: 0
- type: float
  description: modulation frequency as ratio of carrier
  default: 1
- type: float
  description: modulation index
  default: 0

inlets:
  1st:
  - type: float/signal
    description: carrier frequency in hz
  2nd:
  - type: float/signal
    description: modulation frequency as ratio of carrier
  3rd:
  - type: float/signal
    description: modulation index
    
draft: false

---

[fm~] is a simple frequency modulation unit consisting of a pair of sinusoidal oscillators, where one modulates the frequency input of another. The modulation frequency is set as a ratio of the carrier and the modulation index is multiplied by this resulting frequency. The object has support for multichannels.