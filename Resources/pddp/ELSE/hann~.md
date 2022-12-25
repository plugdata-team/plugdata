---
title: hann~

description: Hann window

categories:
- object

pdcategory:

arguments:

inlets:
  1st:
  - type: signal
    description: input to hann window

outlets:
  1st:
  - type: signal
    description: output of hann window

draft: false
---

[hann~] is an abstraction that applies a hann window into a signal input. the window size is the same as the block size. This is meant for FFT analysis.

