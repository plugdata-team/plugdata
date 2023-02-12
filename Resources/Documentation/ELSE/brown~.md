---
title: brown~

description: brown noise generator

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators, Signal Math

arguments:
- type: float
  description: maximum step 
  default: 0.125

inlets:
  1st:
  - type: signal
    description: impulses get a new random step
  - type: float
    description: sets maximum random step (from 0 to 1)

outlets:
  1st:
  - type: signal
    description: brown noise

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[brown~] is a brown noise generator (aka brownian noise or red noise), whose spectral energy drops 6dB per octave (sounding less hissy than white noise). It is implemented as a bounded random walk (based on a pseudo random number generator algorithm). By default, the maximum step is 0.125, but you can set that. If a signal is connected, non-0 values generate new random steps.
