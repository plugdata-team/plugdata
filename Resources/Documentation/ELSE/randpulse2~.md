---
title: randpulse2~

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
  - type: list/signals
    description: density

outlets:
  1st:
  - type: signals
    description: random pulse signal

flags:
  - name: -seed <float>
    description: seed value
  - name: -ch <float>
    description: number of output channels
    default: 1
  - name: -mc <list>
    description: sets multichannel output with a list of frequencies

methods:
  - type: rand <float>
    description: non-0 sets to random gate value mode
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  - type: set <float, float>
    description: <channel, freq> set a single density channel

draft: false
---

[randpulse2~] is a random pulse train oscillator. It alternates at random intervals according to a "density" parameter, which controls the average frequency.
