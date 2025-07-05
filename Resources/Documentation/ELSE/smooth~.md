---
title: smooth~

description: signal smoothner

categories:
- object

pdcategory: ELSE, Signal Math

arguments:
- type: float
  description: glide time in ms
  default: 0

flags:
  - type: -curve <float>
    description: sets curve factor (default '0', linear)

methods:
  - type: reset
    description: resets to input value
  - type: curve <float>
    description: sets curve factor

inlets:
  1st:
  - type: float/signals
    description: incoming signal to smooth out
  2nd:
  - type: signals
    description: glide time in ms

outlets:
  1st:
  - type: signals
    description: smoothned signal

draft: false
---

[smooth~] smoothens a signal transition with linear interpolation by default, at a given time in ms. You can also have exponential curves and there's support for multichannel signals.
