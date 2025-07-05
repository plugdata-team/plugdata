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

methods:
  - type: clear
    description: clears filter's memory
  - type: q
    description: sets resonance parameter to Q (default)
  - type: bw
    description: sets resonance parameter to bandwidth in octaves
  - type: t60
    description: sets resonance parameter to decay time in ms

inlets:
  1st:
  - type: signal
    description: signal to be filtered or excite the resonator
  2nd:
  - type: float/signal
    description: central frequency in Hz
  3rd:
  - type: signal
    description: resonance (Q, bw or t60)

outlets:
  1st:
  - type: signal
    description: resonator/filtered signal

  methods:
  - type: clear
    description: clears filter's memory
  - type: bypass <float>
    description: 1 (bypass on), 0 (bypass off)
  - type: t60
    description: sets resonance parameter in decay time in ms
  - type: bw
    description: sets resonance parameter to bandwidth in octaves
draft: false
---

[resonant~] is a bandpass resonator filter like [bandpass~], but it doesn't have a maximum dB value of 0, so changing the Q increases the gain of the filter. Besides 'Q' and 'bw' you can also set the resonance as 't60' (like with [resonator~]).
