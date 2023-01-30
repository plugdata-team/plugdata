---
title: hann~

description: Hann window

categories:
- object

pdcategory: ELSE, Analysis, Effects, Signal Math

arguments:

inlets:
  1st:
  - type: signal
    description: input to Hann window

outlets:
  1st:
  - type: signal
    description: output of Hann window

draft: false
---

[hann~] is an abstraction that applies a Hann window into a signal input. the window size is the same as the block size. This is meant for FFT analysis.

