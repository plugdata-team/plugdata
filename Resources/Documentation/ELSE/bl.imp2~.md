---
title: bl.imp2~

description: bandlimited two-sided impulse oscillator

categories:
- object

pdcategory: Signal Generators

arguments:
  - type: float
    description: frequency in Hz
    default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
outlets:
  1st:
  - type: signal
    description: two sided impulse signal

draft: false
---

[bl.imp2~] is a two sided impulse oscillator like [else/imp2~], but it is bandlimited with the upsampling/filtering technique. This makes the object quite inefficient CPU wise, but is an easy way to implement a bandlimited oscillator. 

