---
title: rampnoise.mc~

description: multichannel rampnoise bank

categories:
 - object

pdcategory: ELSE, Signal Generators, Random and Noise

arguments:
- type: list
  description: list of frequencies

flags:
  - name: -sum
    description: sums the rampnoises 
  - name: -seed <float>
    description: sets seed

inlets: 
  1st:
  - type: signals
    description: frequency values

outlets:
  1st:
  - type: signals
    description: multichannel signal from ramp noise bank

methods:
  - type: sum
    description: non zero sums the ramp noises into a single channel
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[rampnoise.mc~] takes multichannel signals to set frequencies of different rampnoise generators. The output is also a multichannel signal by default, but you can “sum” them into a single channel with the “sum” flag or message.

