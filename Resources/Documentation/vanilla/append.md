---
title: append
description: add an item to a data structure list
see_also: 
- pointer
- getsize
- setsize
- get
- set
- scalar
- struct
- element  
pdcategory: vanilla, Data Structures, UI
last_update: '0.47'
inlets:
  1st:
  - type: float
    description: set field value and append new data structure
  nth:
  - type: float
    description: set field value
  2nd: #rightmost
  - type: pointer
    description: a pointer to the data structure to add
outlets:
  1st:
  - type: pointer
    description: a pointer to added data structure
arguments:
- type: symbol
  description: set template name
- type: list
  description: set field names and create an inlet for each (one inlet is created by default)
methods:
  - type: set <symbol, symbol>
    description: if none or just one field is given, you can use 'set' to set struct name and field
draft: true #those variable middles are everywhere it seems

---
"append" maintains a pointer to a scalar, or else an empty pointer to the head of a list. You may set the pointer using the leftmost inlet. The creation arguments specify the template of a new scalar to append, and the names of the fields (there should be at least one) you will wish to initialize. To append an object, send a number to the leftmost inlet. "Append"'s pointer is updated to point to the new scalar, and the new pointer is also output.

To insert to the beginning of a list, you can append to the "head" of the list.

If there are no fields or just one specified as as argument, you can use a "set" message to set both the template and field.
