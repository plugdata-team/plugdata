---
title: stepnoise.mc~

description: multichannel stepnoise bank

categories:
 - object
 
pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: list
  description: list of frequencies

flags:
- name: -sum
  description: sums the step noises
- name: -seed <float>
  description: sets seed 
  default: unique internal

inlets:
  1st:
  - type: signals
    description: frequency values

outlets:
  1st:
  - type: signal
    description: bandlimited step noise

methods:
  - type: sum
    description: non zero sums the step noises into a single channel
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[stepnoise.mc~] takes multichannel signals to set frequencies of different stepnoise generators. The output is also a multichannel signal by default, but you can “sum” them into a single channel with the “sum” flag or message.
