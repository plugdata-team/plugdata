---
title: bl.imp~

description: bandlimited impulse oscillator

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: frequency in Hz
    type: float
    default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz

outlets:
  1st:
  - type: signal 
    description: impulse oscillator signal

draft: false
---

[bl.imp~] is an impulse oscillator like [else/imp~], but it is bandlimited.
