---
title: allpass.2nd~

description: Allpass filter

categories:
 - object

pdcategory: General

arguments:
  1st:
  - type: float
    description: central frequency in Hz
    default: 0
  2nd:
  - type: float
    description: resonance
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
  - type: float/signal
    description: central frequency in Hz
  3rd:
  - type: float/signal
    description: filter resonance (Q or bandwidth)

outlets:
  1st:
  - type: signal
    description: filtered signal

draft: false
---

[allpass.2nd~] is a 2nd order allpass filter.
