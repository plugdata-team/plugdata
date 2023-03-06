---
title: decay2~
description: exponential decay

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: attack in ms
  default: 100
- type: float
  description: decay in ms
  default: 1000

inlets:
  1st:
  - type: float
    description: control trigger
  - type: bang
    description: control trigger
  - type: signal
    description: impulse trigger
  2nd:
  - type: float/signal
    description: attack time in ms
  3rd:
  - type: float/signal
    description: decay time in ms

outlets:
  1st:
  - type: signal
    description: decayed signal

methods:
  - type: clear
    description: clears filter's memory

draft: false
---

[decay2~] is like [decay~], but it has an attack time parameter. It is based on SuperCollider's "Decay2" UGEN.

