---
title: gbman~
description: gingerbread man chaotic generator

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets initial feedback value of y[n-1]
  default: 1.2
- type: float
  description: initial feedback value of y[n-2]
  default: 2.1

flags:
  - name: -mc <list>
    description: sets multichannel output with a list of frequencies

inlets:
  1st:
  - type: list
    description: frequencies as MIDI
  - type: signals
    description: frequencies as Hz (negative values accepted)
  - type: set <float, float>
    description: <channel, freq> set a single frequency channel

outlets:
  1st:
  - type: signals
    description: gingerbread man map chaotic signal(s)

draft: false
---

[gbman~] is a gingerbread man map chaotic generator, the output is from the difference equation => y[n] = 1 + abs(y[n-1]) - y[n-2].
