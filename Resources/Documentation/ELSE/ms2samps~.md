---
title: ms2samps~

description: convert ms to samples

categories:
- object

pdcategory: ELSE, Converters

arguments:

inlets:
  1st:
  - type: float/signal
    description: time value in ms

outlets:
  1st:
  - type: signal
    description: converted value in number of samples

draft: false
---

[ms2samps] is a simple abstraction that converts time values from ms to number of samples. 
Conversion expression: samps = ms * samplerate/1000.
