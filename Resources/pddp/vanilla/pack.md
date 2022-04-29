---
title: pack
description: make compound messages
categories:
- object
pdcategory: General
last_update: '0.34'
see_also:
- trigger
- unpack
arguments:
- description: list of types (defining the number of inlets). These can be 'float/'f',  'symbol/'s'
    and 'pointer/'p'. A number sets a numeric inlet and initializes the value,  'float/f'
    initialized to 0 (default 0 0).
  type: list
inlets:
  1st:
  - type: anything
    description: each inlet takes a message type acccording to its corresponding creation
      argument. These can be float,  symbol and pointer. The 1st inlet causes an output
      and can also match an 'anything' to a symbol.
  - type: bang
    description: output the packed list.
  'n: (number depends on number of arguments)':
outlets:
  1st:
  - type: list
    description: the packed list.
draft: false
---
The pack object outputs a concatenated list from a series of inputs. Creation arguments set the number of inlets and their type,  possible values are: float (or 'f'),  symbol (or 's') and pointer (or 'p') - see [pd pointer]. A number sets a numeric inlet and initializes the value ('float'/'f' initializes to 0).

The [pack] object can pack a pointer into a list. A pointer can be the location of a Data Structure scalar somewhere or the head of a Data Structure list. To know more about Data Structures,  how to handle pointers and see examples,  please refer to the 4.Data.Structure section of the Pd's tutorials.
