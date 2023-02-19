---
title: dust~

description: random impulses

categories:
 - object

pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: density
  default: 0

inlets:
  1st:
  - type: float/signal
    description: density (rate) of random impulses

outlets:
  1st:
  - type: signal
    description: random impulses

flags:
  - name: -seed <float>
    description: seed value (default: unique internal)

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[dust~] is based on SuperCollider's "Dust" UGEN and outputs random impulse values (only positive values up to 1) at random times according to a density parameter. The difference to SuperCollider's is that it only produces actual impulses (one non-0 value in between 0 valued samples).

