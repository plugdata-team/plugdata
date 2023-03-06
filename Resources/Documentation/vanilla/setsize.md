---
title: setsize
description: resize a data structure array
categories:
- object
pdcategory: vanilla, Data Structures

arguments:
- type: symbol
  description: template name
- type: symbol
  description: field name
inlets:
  1st:
  - type: float
    description: set the array size
  2nd:
  - type: pointer
    description: pointer to a scalar with an array field
outlets:

methods:
  - type: set <symbol, symbol>
    description: set template and field name
draft: false
---

