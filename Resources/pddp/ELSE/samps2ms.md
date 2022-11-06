---
title: samps2ms

description: Convert samples to ms

categories:
- object

pdcategory: Math (Conversion)

arguments: (none)

inlets:
  1st:
  - type: float
    description: time value in number of samples

outlets:
  1st:
  - type: float
    description: converted value in ms

draft: false
---

[samps2ms] is a simple abstraction that converts time values number of samples to ms. Conversion expression: ms = 1000*samps/ samplerate.
