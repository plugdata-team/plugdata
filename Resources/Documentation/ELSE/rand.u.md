---
title: rand.u

description: unrepeatable random values

categories:
- object

pdcategory: ELSE, Random and Noise

arguments:
- type: float
  description: size
  default: 1

inlets:
  1st:
  - type: bang
    description: generates a random output
  2nd:
  - type: float
    description: clears memory and sets a new size

outlets:
  1st:
  - type: float
    description: unrepeated random float
  2nd:
    - type: bang
        description: bang when sequence is finished


flags:
  - name: -seed <float>
    description: sets seed

methods:
  - type: clear
    description: clears memory
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  - type: size <float>
    description: clears the memory and sets a new 'n' size

draft: false
---

[rand.u] generates an unrepeated random values (from 0 to size-1). After the whole sequence is output, a bang is sent in the right outlet.
