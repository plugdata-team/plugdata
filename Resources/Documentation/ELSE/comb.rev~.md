---
title: comb.rev~

description: Comb reverberator

categories:
 - object

pdcategory: DSP (Reverberation)
arguments:
  1st:
  - type: float
    description: maximum and initial delay time in ms
    default: 0
  2nd:
  - type: float
    description: a- input gain coefficient
    default: 0
  3rd:
  - type: float
    description: b- feedforward gain coefficient
    default: 0
  3rd:
  - type: float
    description: c- feedback gain coefficient
    default: 0

inlets:
  1st:
  - type: signal
    description: signal input to the filter
  - type: size <float>
    description: changes the maximum delay size (in ms)
  - type: clear
    description: clears the delay buffer
  2nd:
  - type: float/signal
    description: D- delay time in ms
  3rd:
  - type: float/signal
    description: a- input gain coefficient
  4th:
  - type: float/signal
    description: b- feedforward gain coefficient
  5th:
  - type: float/signal
    description: c- feedback gain coefficient

outlets:
  1st:
  - type: signal
    description: output from comb filter

draft: false
---

Use [comb.rev~] as a reverberator, comb filter and delay.