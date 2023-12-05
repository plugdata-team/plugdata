---
title: list.inc

description: Generate incremented lists

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: start value
  default: 0
- type: float
  description: increment step value 
  default: 0
- type: float
  description: number of elements in the list
  default: 1

inlets:
  1st:
  - description: set start value and generate list
    type: float
  - description: generate list with current values
    type: bang
  2nd:
  - description: increment step value (linear or ratio)
    type: float
  3rd:
  - description: number of elements in the list
    type: float

outlets:
  1st:
  - description: the incremented list
    type: list

flags:
  - name: -r
    description: sets to ratio mode

methods:
  - type: ratio <float>
    description: nonzero sets to 'ratio' mode for given step

draft: false
---

Use [list.inc] to generate a list by giving it a start value, a step value and number of elements. In 'ratio' mode the step is considered a ratio step that it multiplies to, otherwise it's just a regular linear addition.
