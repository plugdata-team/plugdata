---
title: noise~
description: uniformly distributed white noise
categories:
- object
see_also: {}
pdcategory: vanilla, Random and Noise, Signal Generators
last_update: '0.48-2'
inlets:
  1st:
  - type: seed <float>
    description: set seed for random number generator
outlets:
  1st:
  - type: signal
    description: white noise signal (in the range from -1 to 1)


draft: false
---
Noise~ is a random number generator that outputs white noise from a pseudo-random number generator at the audio rate (with output from -1 to 1).
