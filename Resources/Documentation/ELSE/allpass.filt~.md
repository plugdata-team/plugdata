---
title: allpass.filt~

description: allpass filter

categories:
- object

pdcategory: ELSE, Filters

arguments:
  - description: order from 2 to 64
    type: float
    default: 2
  - description: frequency in Hz
    type: float
    default: 10
  - description: filter Q
    type: float
    default: 1

inlets:
  1st:
  - type: signal
    description: signal to be filtered
  2nd:
  - type: float/signal
    description: central frequency in Hz
  3rd:
  - type: float/signal
    description: filter resonance (Q)

outlets:
  1st:
  - type: signal
    description: filtered signal

draft: false
---

[allpass.filt~] is an allpass filter whose order can be set via the first argument (must be a multiple of 2). This is an abstraction that stacks many [allpass.2nd~] objects in cascade.
