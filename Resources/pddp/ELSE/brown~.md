---
title: brown~

description: Brown noise generator

categories:
 - object

pdcategory: Audio Oscillators and Tables

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
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

outlets:
  1st:
  - type: signal
    description: brown noise

draft: false
---

[brown~] is a brown noise generator (aka brownian noise or red noise), whose spectral energy drops 6dB per octave (sounding less hissy than white noise). It is implemented as a bounded random walk (based on a pseudo random number generator algorithm). By default, the maximum step is 0.125, but you can set that. If a signal is connected, non zero values generate new random steps.