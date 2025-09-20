---
title: velvet~

description: velvet noise generator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in Hz
  default: 0
- type: float
  description: polarity bias
  default: 0.5
- type: float
  description: time regularity
  default: 0
- type: float
  description: amplitude irregularity
  default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in Hz
  2nd:
  - type: float/signal
    description: polarity bias (0-1)
  3rd:
  - type: float/signal
    description: time regularity (0-1)
  4th:
  - type: float/signal
    description: amplitude irregularity (0-1)

outlets:
  1st:
  - type: signal
    description: velvet noise signal

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

flags:
- name: -seed <float>
  description: sets random seed
  default: unique internal
- name: -ch <float>
  description: sets number of channels
  default: 1

draft: false
---
[velvet~] is a velvet noise generator, which randomly chooses either positive (1) or negative (-1) impulses at random positions in a given period set in Hz. A polarity bias is possible to set the amount of positive and negative impulses. A time regularity parameter forces a impulses with less randomness. An amplitude irregularity forces random values (different than 1 or -1). The object has support for multichannel.
