---
title: decay~
description: exponential decay

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: float
  description: decay time in ms
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

[decay~] is based on SuperCollider's "Decay" UGEN. It is a one pole filter that creates an exponential decay from impulses. The decay time (in ms) is how long it takes for the signal to decay 60dB. The same filter is also used in other objects from the ELSE library (asr~/adsr~/lag~/lag2~).

