---
title: smooth2~

description: signal smoothner

categories:
- object

pdcategory: ELSE, Signal Math

arguments:
- type: float
  description: ramp up time in ms
  default: 0
- type: float
  description: ramp down time in ms
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
    description: ramp up time in ms
  3rd:
  - type: signals
    description: ramp down time in ms

outlets:
  1st:
  - type: signals
    description: smoothned signal

draft: false
---

[smooth2~] is just like [smooth~] but has distinct ramp times for both up and down.
