---
title: resonant~

description: bandpass resonant filter (constant skirt)

categories:
 - object

pdcategory: ELSE, Filters, Effects

arguments:
- type: float
  description: center frequency in Hz
  default: 0
- type: float
  description: resonance
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be filtered or excite the resonator
  2nd:
  - type: float/signal
    description: central frequency in Hz
  3rd:
  - type: signal
    description: resonance (t60 decay time in ms or Q)

outlets:
  1st:
  - type: signal
    description:

  methods:
  - type: clear
    description: clears filter's memory
  - type: bypass <float>
    description: 1 (bypass on), 0 (bypass off)
  - type: t60 
    description: sets resonance parameter in decay time in ms (default)
  - type: q
    description: sets resonance parameter to Q

draft: false
---

[resonant~] is a resonator that you can specify a decay time in ms, or a Q factor. Like [bandpass~], it is a 2nd order bandpass resonant filter, but changing the Q increases the gain of the filter.
