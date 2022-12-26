---
title: ms2samps

description: Convert ms to samples

categories:
- object

pdcategory: Math

arguments: (none)

inlets:
  1st:
  - type: float
    description: time value in ms

outlets:
  1st:
  - type: float
    description: converted value in number of samples

draft: false
---

[ms2samps] is a simple abstraction that converts time values from ms to number of samples.
Conversion expression: samps = ms * samplerate/1000.