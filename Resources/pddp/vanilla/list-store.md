---
title: list store
description: manipulate lists
categories:
- object
pdcategory: General
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
    description: concatenate incoming list with stored list and output (a bang is
      a zero element list and outputs stored list