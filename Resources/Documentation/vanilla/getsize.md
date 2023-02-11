---
title: getsize
description: get size of a data structures array
categories:
- object
see_also: 
- struct
- pointer
- element
- setsize
- array get 
pdcategory: vanilla, Data Structures
last_update: '0.47'
inlets:
  1st:
  - type: pointer 
    description: pointer to a scalar with an array field
  - type: set <symbol, symbol>
    description: set template and field name
outlets:
  1st:
  - type: float
    description: array's size
arguments:
- type: symbol
  description: template name
- type: symbol
  description: field name
draft: false
---
When sent a pointer of a scalar, "getsize" looks up a field, which should be an array, and outputs the number of elements of the array. The template and field name are specified as creation arguments. The smallest possible size is one.

If you don't know the template name you may specify "-", in which case the object will figure out the template name itself, at some possible cost in efficiency and clarity.
