---
title: list.seq

description: sequence a list

categories:
 - object
pdcategory: ELSE, Data Management

arguments:
- type: float
  description: list to sequence through
  default: 0

inlets:
  1st:
  - description: list to sequence through
    type: list
  - description: output an element list
    type: bang

outlets:
  1st:
  - description: the generated incremented list
    type: list
  2nd:
  - description: when the sequence is done
    type: bang

methods:
  - type: goto <float>
    description: go to an element index (indexed from 1)
  - type: loop <float>
    description: nonzero sets to loop mode

flags:
  - name: -loop
    description: sets to loop mode

draft: false
---

Use [list.seq] to sequence through a list, looping through it.
