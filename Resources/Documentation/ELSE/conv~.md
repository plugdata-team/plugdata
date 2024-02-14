---
title: conv~

description: partitioned convolution

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: impulse response table name 
  type: symbol
  default: none
- description: optional - partition size
  type: float
  default: 1024, minimum 64

inlets:
  1st:
  - type: signal
    description: input signal

outlets:
  1st:
  - type: signal
    description: output signal

methods:
  - type: size <float>
    description: sets partition size
  - type: set <symbol>
    description: load IR table
  - type: print
    description: print convolution info to console

draft: false
---

[conv~] is an abstraction that performs partitioned convolution. It takes a partition size and Impulse Response sound file.
