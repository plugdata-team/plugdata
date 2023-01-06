---
title: bandpass~

description: bandpass resonant filter (constant gain)

categories:
 - object

pdcategory: Audio Filters

arguments:
- type: float
  description: central frequency in Hz
  default: 0
- type: float
  description: resonanace, either in Q (default) or bandwidth
  default: 1

flags:
- name: -bw
  description: sets resonance parameter to bandwidth in octaves

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  - type: clear
    description: clears filter's memory if you blow it up
  - type: bypass <float>
    description: 1 (bypasses input signal) or 0 (doesn't bypass)
  - type: bw
    description: sets resonance parameter to bandwidth in octaves
  - type: q
    description: sets resonance parameter to Q (default)
  2nd:
  - type: signal
    description: central frequency in Hertz
  3rd:
  - type: signal
    description: filter resonance (Q or bandwidth)

outlets:
  1st:
  - type: signal
    description: filtered signal

draft: false
---

[bandpass~] is a 2nd order bandpass resonant filter. Unlike [else/resonant~], it has a maximum and constant gain at 0dB.