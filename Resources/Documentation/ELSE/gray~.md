---
title: gray~
description: gray code noise generator

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:

inlets:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

outlets:
  1st:
  - type: signal
    description: gray noise

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[gray~] generates noise based on "gray code" or reflected binary code (RBC), which results from flipping random bits (sot is is based on a pseudo random number generator algorithm.). "Gray Code" is named after Frank Gray, the owner of the patent of gray codes. This type of noise has a high RMS level relative to its peak to peak level. The spectrum is emphasized towards lower frequencies.

