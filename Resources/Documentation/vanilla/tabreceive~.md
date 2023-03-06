---
title: tabreceive~
description: read a block of a signal from an array continuously
categories:
- object
see_also:
- send~
- block~
- array
- tabwrite~
- tabsend~
pdcategory: vanilla, Mixing and Routing, Arrays and Tables
last_update: '0.43'
inlets:
  1st:
  - type: set <name>
    description: set table name
outlets:
  1st:
  - type: signal
    description: outputs signal from a matching tabsend~ object
arguments:
  - type: symbol
    description: receive name symbol 
    default: empty symbol
draft: false
---
By default a block is 64 samples but this can be changed with the block~ object.
