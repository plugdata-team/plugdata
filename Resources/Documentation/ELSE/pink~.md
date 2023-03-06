---
title: pink~

description: pink noise

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: number of octaves
  default: depends on sample rate

inlets:
  1st:
  - type: float
    description: set number of octaves (minimum 1, max 31)

outlets:
  1st:
  - type: signal
    description: pink noise

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[pink~] is a pink noise generator, which sounds less hissy than white noise (but not as less as brown~). White noise has constant spectral power, but pink noise has constant power per octave and it decrease 3dB per octave. Like other noise objects, this is based on a pseudo random number generator algorithm.

