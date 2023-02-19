---
title: list store
description: manipulate lists
categories:
- object
pdcategory: vanilla, Data Management
see_also:
- list
- list append
- list prepend
- list split
- list trim
- list length
- list fromsymbol
- list tosymbol
arguments:
- description: initialize the stored list 
  default: empty
  type: list
inlets:
  1st:
  - type: list
    description: concatenate incoming list with stored list and output (a bang is a zero element list and outputs stored list)
  2nd:
  - type: anything
    description: set stored list (a bang is a zero element list and clears it)
outlets:
  1st:
  - type: list
    description: the stored list at bangs, the concatenated list at list input or sublists via 'get' message
  2nd:
  - type: bang
    description: when get message fails
methods:
  - type: prepend <list>
    description: prepend a list to the stored list
  - type: append <list>
    description: append a list to the stored list
  - type: get <list>
    description: output an item (if only one float is given) or sublist, where first element sets staring index and the second sets ending index (-1 is end of the list)
  - type: set <list>
    description: set values starting at index from the first element
  - type: insert <list>
    description: insert values before index from the first element
  - type: delete <list>
    description: delete a given item for just one float or a number of items specified in the second element starting at index from the first element (-1 means delete all items from given index)
  - type: send <symbol>
    description: send stored list to a named receiver
draft: false
---
put together or break apart a list to/from sublists
