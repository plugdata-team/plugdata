---
title: randpulse2

description: random pulse train oscillator

categories:
- object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: density
  default: 0
- type: float
  description: non-0 sets to random gate value mode
  default: 0

inlets:
  1st:
  - type: float/signal
    description:

outlets:
  1st:
  - type: signal
    description: random pulse signal

flags:
  - name: -seed <float>
    description: sets seed

methods:
  - type: rand <float>
    description: non-0 sets to random gate value mode
  - type: seed <float>
    description: - a float sets seed, no float sets a unique internal

draft: false
---

[randpulse2] is a random pulse train oscillator (which alternates between 0 and 1 at random intervals according to a "density" parameter, which controls the average frequency.
