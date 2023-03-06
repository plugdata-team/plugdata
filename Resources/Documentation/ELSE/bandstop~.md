---
title: bandstop~

description: bandstop filter

categories:
 - object

pdcategory: ELSE, Filters

arguments:
  - type: float
    description: central frequency in Hz
    default: 0
  - type: float
    description: resonance, either in 'Q' (default) or 'bw'
    default: 1

flags:
- name: -bw
  description: sets resonance parameter to bandwidth in octaves

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

methods:
  - type: clear
    description: clears filter's memory if you blow it up
  - type: bypass <float>
    description: 1 (bypasses input signal) or 0 (doesn't bypass)
  - type: bw
    description: sets resonance parameter to bandwidth in octaves
  - type: q
    description: sets resonance parameter to Q

draft: false
---

[bandstop~] is a 2nd order band reject filter, it can also be used as a 'notch' filter.
