---
title: var

description: store one or more variables

categories:
 - object
pdcategory: ELSE, Mixing and Routing

arguments:
- type: list
  description: list of variable names
  default: none

inlets:
  1st:
  - description: input values to given variables
    type: list

outlets:
  1st:
  - description: values to given variables
    type: list

draft: false
---

[var] is similar to [value] but it can set and recall more than one variable as lists.
