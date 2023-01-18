---
title: lores~
description: Low-pass resonant filter
categories:
 - object
pdcategory: General
arguments:
- type: float
  description: cutoff frequency
  default:
- type: float
  description: resonance: minimum is 0 (a little bit sharp) and maximum is 1 (as sharp as possible and also LOUD)
  default: 0
inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float/signal
    description: cutoff frequency
  3rd:
  - type: float/signal
    description: resonance
outlets:
  1st:
  - type: signal
    description: the filter's output

methods:
  - type: clear
    description: clears the internal buffer's memory

---

[lores~] implements an inexpensive resonant lowpass filter. The first argument or middle inlet sets the cutoff frequency, and the resonance is set by the second argument or the right inlet. Below, we use a LFO to control the cutoff, resulting in a filter sweep effect.

