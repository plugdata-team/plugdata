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
    of which can be abreviatted (default: f f).'
  type: list
inlets:
  1st:
  - type: list
    description: a list to be split into atoms.
outlets:
  'n: (depends on the number of arguments)':
  - type: float/symbol
    description: a float or a symbol,  depending on the argument.
draft: false
---
[unpack] takes lists of atoms and distributes them to its outlets. The creation arguments specify the number of outlets and the atom type,  possible values are: float (or 'f'),  symbol (or 's') and pointer (or 'p') - see [pd pointer].

The [unpack] object unpack a pointer from a list. A pointer can be the location of a Data Structure scalar somewhere or the head of a Data Structure list. To know more about Data Structures,  how to handle pointers and see examples,  please refer to the 4.Data.Structure section of the Pd's tutorials.
