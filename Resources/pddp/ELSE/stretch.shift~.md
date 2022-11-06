---
title: stretch.shift~

description: Pitch-Shift Time-Stretch

categories:
- object

pdcategory:

arguments:
- type: float
  description: sets buffer size in ms
  default: 5000
- type: float
  description: sets playing speed
  default: 100
- type: float
  description: sets transposition in cents
  default: 0
- type: float
  description: sets grain size in ms
  default: 25

inlets:
  1st:
  - type: signal
    description: audio signal input
  - type: bang
    description: resets to the beginning of the delay line
  - type: size <f>
    description: sets buffer size in ms
  2nd:
  - type: float
    description: sets grain size
  3rd:
  - type: float
    description: sets speed
  4th:
  - type: float
    description: sets transposition in cents

outlets:
  1st:
  - type: signal
    description: the buffer signal of the corresponding channel

draft: false
---

[stretch.shift~] is like [gran.player~], but for live input. It provides independent time stretching and pitch shifting via granulation.
