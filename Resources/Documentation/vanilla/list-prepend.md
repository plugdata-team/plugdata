---
title: list prepend
description: manipulate lists
categories:
- object
pdcategory: vanilla, Data Management
see_also:
- list
- list append
- list store
- list split
- list trim
- list length
- list fromsymbol
- list tosymbol
arguments:
- description: initialize the list to prepend 
  default: empty
  type: list
inlets:
  1st:
  - type: anything
    description: set messages to be prepended by a second list and output (a bang is a zero element list)
  2nd:
  - type: anything
    description: set messages to prepend to the first list (a bang is a zero element list and clears it)
outlets:
  1st:
  - type: list
    description: the prepended list
draft: false
---
prepend a second list to the first
