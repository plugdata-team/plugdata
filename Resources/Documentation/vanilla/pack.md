---
title: pack
description: make compound messages
categories:
- object
pdcategory: vanilla, Data Management
last_update: '0.34'
see_also:
- trigger
- unpack
arguments:
- description: symbols defining types of inlets: float/symbol/pointer (f/s/p). a number sets a numeric inlet and initializes the value. f is initialized to 0
  default: 0 0
  type: list
inlets:
  1st:
  - type: anything
    description: message type according to corresponding argument (float/symbol/pointer). 1st inlet causes an output, and can match an 'anything' to a symbol
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

