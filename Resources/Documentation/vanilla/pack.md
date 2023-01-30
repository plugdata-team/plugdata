---
title: pack
description: make compound messages
categories:
- object
pdcategory: vanilla,  Data Management
last_update: '0.34'
see_also:
- trigger
- unpack
arguments:
- description: list of types (defining the number of inlets). These can be 'float/f', 'symbol/s', and 'pointer/p'. A number sets a numeric inlet and initializes the value, 'float/f' initialized to 0
  default: 0 0
  type: list
inlets:
  1st:
  - type: anything
    description: message type according to corresponding creation argument. These can be float, symbol, and pointer. The 1st inlet causes an output, and can also match an 'anything' to a symbol
  - type: bang
    description: output the packed list
  nth:
  - type: anything
    description: message type according to corresponding creation argument. These can be float, symbol, and pointer
outlets:
  1st:
  - type: list
    description: the packed list

---

combine several atoms into one message

