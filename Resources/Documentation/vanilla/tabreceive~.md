---
title: tabreceive~
description: read a block of a signal from arrays continuously
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
  - type: set <list>
    description: set table names
outlets:
  1st:
  - type: signal
    description: outputs 1 or more channels from matching arrays
arguments:
  - type: list
    description: table names
    default: empty symbol
draft: false
---
By default a block is 64 samples but this can be changed with the block~ object.
