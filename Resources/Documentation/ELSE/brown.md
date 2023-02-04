---
title: brown

description: Brownian motion

categories:
 - object

pdcategory: ELSE, Random and Noise, Data Math

arguments:
- type: float
  description: maximum step 
  default: 0.125

inlets:
  1st:
  - type: bang
    description: generate new random step
  - type: float
    description: sets maximum random step (from 0 to 1)

outlets:
  1st:
  - type: float
    description: random value based on brownian motion

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[brown] is a control brownian motion generator. It is a bounded random walk (based on a pseudo random number generator algorithm). By default, the maximum step is "0.125", which is a ratio of the whole range (and the output range is from 0 to 127). If it tries to step outside the bounds, it just gets folded back.
