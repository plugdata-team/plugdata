---
title: select~

description: select inputs

categories:
 - object
 
pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: number of input signals
  default: 2
- type: float
  description: initially selected input
  default: 0

inlets:
  1st:
  - type: float
    description: input to select
  nth:
  - type: signal(s)
    description: input signals to select from

outlets:
  1st:
  - type: signal(s)
    description: the selected input

draft: false
---

[select~] selects an input signal without crossfade and the signals can be of different channel sizes. The number of inputs is set via the 1st argument. The 2nd argument sets the initial input (indexed from 1). The selection is clipped to the number of inputs.