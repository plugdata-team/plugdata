---
title: unpack
description: get elements of compound messages
categories:
- object
pdcategory: General
last_update: '0.33'
see_also:
- pack
- trigger
arguments:
- description: 'symbols that define atoms''s type: float'',  ''symbol'',  and ''pointer'',  all
    of which can be abreviatted 
  default:: f f
.'
  type: list
inlets:
  1st:
  - type: list
    description: a list to be split into atoms.
outlets:
  'n: (depends on the number of arguments