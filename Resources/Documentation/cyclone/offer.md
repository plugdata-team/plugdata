---
title: offer

description: store one-timer number pairs

categories:
 - object

pdcategory: cyclone, Data Management

arguments: (none)

inlets:
  1st:
  - type: float
    description: sets 'x' value, outputs 'y' value, destroys pair
  - type: list
    description: x/y pair to store
  - type: bang
    description: outputs every stores 'y' value
  2nd:
  - type: float
    description: 'y' value, before setting 'x' in the left inlet

outlets:
  1st:
  - type: float
    description: 'y' value corresponding to a given 'x'

methods: 
  - type: clear
    description: clears the stored pairs

draft: true
---

[offer] stores a x/y integer number pair and accesses the 'y' value from the corresponding 'x' (after retrieving, the pair is deleted).
