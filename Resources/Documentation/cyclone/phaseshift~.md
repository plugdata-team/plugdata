---
title: phaseshift~

description: 2nd order Allpass filter

categories:
 - object

pdcategory: cyclone, Filters, Effects

arguments:
- type: float
  description: sets filter frequency
  default: 1
- type: float
  description: sets Q
  default:

inlets:
  1st:
  - type: signal
    description: signal whose phase will be shifted
  2nd:
  - type: signal
    description: sets filter's frequency point to be shifted to 180º
  3rd:
  - type: signal
    description: sets Q

outlets:
  1st:
  - type: signal
    description: the phase shifted signal

methods:
  - type: clear
    description: clears filter's memory

draft: true
---

[phaseshift~] is a 2nd allpass filter, which keeps the gain and only alters the phase from 0 (at 0 Hz) to 360º (at the Nyquist frequency). The frequency at which it shifts to 180º is specified as the filter's frequency and the steepness of the curve is determined by the Q parameter (see graph below).
In this example, we add the phase shifted signal to the original, which cancels frequencies by phase opposition.
