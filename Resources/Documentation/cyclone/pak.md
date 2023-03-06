---
title: pak

description: output a list when any element changes

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: anything
  description: outlet types (float, int, symbol)
  default: 0, 0

inlets:
  nth:
  - type: bang 
    description: outputs the stored list of elements
  - type: anything
    description: update inlets' values and output them

outlets:
  1st:
  - type: list
    description: the list composed of the given elements

methods:
  - type: set <anything>
    description: updates inlets' values without outputting them

draft: true
---

[pak] (pronounced "pock") is much like pack, but any inlet triggers the output of a list. The message set avoid the output and a bang triggers the output.
