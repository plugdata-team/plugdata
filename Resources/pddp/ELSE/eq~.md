---
title: eq~

description: Parametric equalizer filter

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: central frequency in Hertz
  default: 0
- type: float
  description: resonance, either in 'Q' (default) or 'bw'
  default: 1
- type: float
  description: gain in dB
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float/signal
    description: central frequency in Hertz
  3rd:
  - type: float/signal
    description: filter resonance (Q or bandwidth)
  4th:
  - type: float/signal
    description: gain in dB

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

---

[eq~] is a 2nd order parametric equalizer filter, it can be used as a peak and a notch filter.

