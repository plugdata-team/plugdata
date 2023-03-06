---
title: urn

description: unrepeated random numbers

categories:
 - object

pdcategory: cyclone, Random and Noise

arguments:
- type: float
  description: sets size
  default: 1
- type: float
  description: sets seed
  default: internal random one

inlets:
  1st:
  - type: bang
    description: generates unrepeated random numbers 
  - type: float
    description: generates unrepeated random number
  2nd:
  - type: float
    description: clears the memory and sets size

outlets:
  1st:
  - type: float
    description: unrepeated random number output
  2nd:
  - type: bang
    description: bangs if all numbers have been generated

draft: true
---

[urn] generates random numbers in a range defined by the 'n' size (from 0 to n-1) without repeating them. When all numbers have been output, a bang is sent to the right outlet and it stops generating numbers unless it receives a clear message.
