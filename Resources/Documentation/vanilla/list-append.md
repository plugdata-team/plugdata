---
title: list append
description: manipulate lists
categories:
- object
pdcategory: vanilla, Data Management
see_also:
- list
- list prepend
- list store
- list split
- list trim
- list length
- list fromsymbol
- list tosymbol
arguments:
- description: initialize the list to append 
  default: empty
  type: list
inlets:
  1st:
  - type: anything
    description: set messages to concatenate to a second list and output (a bang is a zero element list)
  2nd:
  - type: anything
    description: set messages to append to the first list (a bang is a zero element list and clears it)
outlets:
  1st:
  - type: list
    description: the concatenated list
draft: false
---
append a second list to the first

