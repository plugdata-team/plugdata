---
title: tabsend~
description: write a block of a signal to arrays continuously
categories:
- object
see_also:
- send~
- block~
- array
- tabwrite~
- tabreceive~
pdcategory: vanilla, Arrays and Tables, Mixing and Routing
last_update: '0.43'
inlets:
  1st:
  - type: signal
    description:  1 or more channels to write to corresponding arrays
arguments:
  - type: list
    description: set table names
    default: empty symbol
methods:
  - type: set <list>
    description: set table names
draft: false
---
By default a block is 64 samples but this can be changed with the block~ object.
