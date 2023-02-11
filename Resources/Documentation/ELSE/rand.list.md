---
title: rand.list

description: choose randomly from a list

categories:
- object

pdcategory: ELSE, Random and Noise

arguments:
- type: list
  description: initial list
  default: 0

inlets:
  1st:
  - type: bang
    description: choose an element from a given list randomly
  2nd:
  - type: list 
    description: sets a new list

outlets:
  1st:
  - type: float
    description: random element from list

flags:
  - name: -seed <float>
    description: sets seed

methods:
  - type: set <list>
    description: sets a new list
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[rand.list] takes a list of floats and randomly chooses from it.
