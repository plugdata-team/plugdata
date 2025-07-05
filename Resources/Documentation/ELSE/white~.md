---
title: white~

description: white noise generator

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:

inlets:
  - type: float
    description: sets seed (no float — unique internal)

outlets:
  1st:
  - type: signals
    description: white noise

flags:
    - name: -seed <float>
      description: sets seed
      default: unique internal
    - name: -clip
      description: sets to clip mode
    - name: -ch <float>
      description: sets number of channels
      default: 1

methods:
    - type: seed <float>
      description: a float sets seed, no float sets a unique internal
    - type: clip
      description: sets to clip mode
    - type: ch <float>
      description: set number of channels

draft: false
---

[white~] is a white noise generator from a pseudo random number generator algorithm.
