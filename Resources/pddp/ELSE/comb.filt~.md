---
title: comb.filt~

description: Comb filter

categories:
 - object

pdcategory: DSP (Filters)

arguments:
  1st:
  - type: float
    description: maximum and initial delay time in ms
    default: 0
  2nd:
  - type: float
    description: decay coefficient
  3rd:
  - type: float
    description: decay mode- <0> or <non-zero> (gain mode)
    default: t60

inlets:
  1st:
  - type: signal
    description: signal input to the filter
  - type: clear
    description: clears the delay buffer
  - type: gain <float>
    description: non zero sets to gain mode instead of 't60'
  2nd:
  - type: float/signal
    description: frequency in hz
  3rd:
  - type: float/signal
    description: decay coefficient

outlets:
  1st:
  - type: signal
    description: output from comb filter

flags:
  - name: -gain
    description: sets feedback mode to gain

draft: false
---

Use [comb.filt~] as a resonator comb filter.