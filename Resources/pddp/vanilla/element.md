---
title: element
description: get pointer to an element of an array
categories:
- object
see_also: 
- struct
- pointer
- getsize
- setsize
- plot
pdcategory: Accessing Data
last_update: '0.47'
inlets:
  1st:
  - type: float
    description: set element index and output pointer.
  - type: set <symbol, symbol>
    description: set template and field name.
  2nd:
  - type: pointer
    description: pointer to an array.
outlets:
  1st:
  - type: pointer 
    description: pointer to element in an array.
arguments:
- type: symbol
  description: template name.
- type: symbol
  description: field name.
draft: false
---
"element" takes a pointer at right and an index number at left. It looks up a field from the pointer, which should be an array, and outputs the element of the array specified by the index number. There are no pointers to arrays themselves, just to individual elements. The template and field name are specified as creation arguments.

Indices range from 0 to the number of elements minus one;
indices out of range are quietly replaced by the nearest endpoint.

If you don't know the template name you may specify "-", in which case the object will figure out the template name itself, at some possible cost in efficiency and clarity.