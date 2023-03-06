---
title: perlin~

description: Perlin noise generator

categories:
- object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- description: sets frequency in Hz
  type: float
  default: Nyquist

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz

outlets:
  1st:
  - type: signal
    description: Perlin noise signal

flags:
  - name: -seed <float>
    description: sets seed (default: unique internal)

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[perlin~] is an abstraction that implements 1-dimensional Perlin Noise (a type of gradient noise developed by Ken Perlin). It uses [white~] as a noise source into a sample and hold function and generates smoothened functions according to a frequency value in Hz (values under 0 and above Nyquist are aliased).

