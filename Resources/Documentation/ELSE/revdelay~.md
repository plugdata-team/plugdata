---
title: revdelay~

description: reverse delay 

categories:
- object

pdcategory: ELSE, Effects

arguments:
- type: float
  description: initial length
  default: 1000 ms

inlets:
  1st:
  - type: signal
    description: signal input into the delay line

outlets:
  1st:
  - type: signal
    description: the feed forward delayed signal

methods:
  - type: resize <float>
    description: changes the delay size
  - type: clear
    description: clears the delay buffer

draft: false
---

[revdelay~] is a reverse delay effect. It takes a size amount in ms to play backwards from a delay line (default 1000 ms).
