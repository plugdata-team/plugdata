---
title: conv~

description: partitioned convolution

categories:
- object

pdcategory: ELSE, Effects

arguments:
- description: optional - partition size
  type: float
  default: 1024, minimum 64
- description: file name to open as impulse response
  type: symbol
  default: none

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
  - type: load <symbol>
    description: loads IR sound file

draft: false
---

[conv~] is an abstraction that performs partitioned convolution. It takes a partition size and Impulse Response sound file.
