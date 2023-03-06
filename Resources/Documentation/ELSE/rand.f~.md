---
title: rand.f~

description: random float number generator

categories:
 - object

pdcategory: ELSE, Signal Math, Random and Noise

arguments:
- type: float
  description: minimum
  default: 0
- type: float
  description: maximum
  default: 1

inlets:
  1st:
  - type: signal
    description: trigger signal
  - type: bang
    description: control rate trigger
  2nd:
  - type: signal
    description: lowest random value
  3rd:
  - type: signal
    description: highest random value

outlets:
  1st:
  - type: signal
    description: random values

flags:
  - name: -seed <float> 
    description: seed value

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[rand.f~] generates random float values for a given range when triggered. A trigger happens at positive to negative or negative to positive transitions. Use the seed message if you want a reproducible output.
