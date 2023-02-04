---
title: rand.f

description: random float number generator

categories:
 - object

pdcategory: ELSE, Data Math, Random and Noise

arguments:
- type: float
  description: minimum
  default: 0
- type: float
  description: maximum
  default: 1

inlets:
  1st:
  - type: bang
    description: generate random number
  2nd:
  - type: float
    description: lowest random value
  3rd:
  - type: float
    description: highest random value

outlets:
  1st:
  - type: float
    description: random values

flags:
  - name: -seed <float> 
    description: seed value

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[rand.f] generates random float values for a given range when triggered with a bang. Use the seed message if you want a reproducible output.
