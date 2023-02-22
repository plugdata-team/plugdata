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
- description: types of inlets: float/symbol/pointer (f/s/p). a number sets a numeric inlet and initializes the value. f is initialized to 0
  default: 0 0
  type: list
inlets:
  1st:
  - type: anything
    description: type according to argument. causes output
  - type: bang
    description: output the packed list
  nth:
  - type: anything
    description: type according to argument
outlets:
  1st:
  - type: list
    description: the packed list
draft: false
---

combine several atoms into one message

