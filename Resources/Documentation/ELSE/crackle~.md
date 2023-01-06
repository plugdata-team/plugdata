---
title: crackle~

description: Crackle Noise generator

categories:
 - object

pdcategory: Noise

arguments:
- type: float
  description: 'p' parameter
  default: 0.5

inlets:
  1st:
  - type: float/signal
    description: chaotic parameter 'p' (from 0 to 1)
  - type: clear
    description: clear filter's memory

outlets:
  1st:
  - type: signal
    description: crackle noise
 
draft: false
---

[crackle~] generates noise based on a chaotic function, defined by the difference equation;
y[n] = (p + 1) * y[n-1] - y[n-2] - 0.05
inspired by SuperCollider's "Crackle" Ugen