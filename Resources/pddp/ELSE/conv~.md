---
title: conv~

description: Partitioned convolution

categories:
- object

pdcategory: DSP (assorted)

arguments:
  1st:
    - description: optional - partition size
      type: float
      default: 1024, minimum 64
  2nd:
    - description: file name to open as impulse response
      type: symbol
      default: none

inlets:
  1st:
  - type: signal
    description: input signal
  - type: size <float>
    description: sets partition size
  - type: load <symbol>
    description: loads IR sound file

outlets:
  1st:
  - type: signal
    description: output signal

draft: false
---

[conv~] is an abstraction that performs partitioned convolution. It takes a partition size and Impulse Response sound file.
