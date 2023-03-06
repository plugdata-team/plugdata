---
title: highpass~
description: highpass resonant filter

categories:
 - object

pdcategory: ELSE, Filters

arguments:
- type: float
  description: central frequency in Hz
  default: 0
- type: float
  description: resonance, either in 'Q' or 'bw'
  default: 1 - in Q

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float/signal
    description: central frequency in Hz
  3rd:
  - type: float/signal
    description: filter resonance (Q or bandwidth)

outlets:
  1st:
  - type: signal
    description: filtered signal

flags:
  - name: -bw
    description: sets resonance parameter to bandwidth in octaves

methods:
  - type: clear
    description: clears filter's memory if you blow it up
  - type: bypass <float>
    description: 1 (bypasses input signal) or 0 (doesn't bypass)
  - type: bw
    description: sets resonance parameter to bandwidth in octaves
  - type: q
    description: sets resonance parameter to Q (default)

draft: false
---

[highpass~] is a 2nd order highpass resonant filter.

