---
title: tabsend~
description: write a block of a signal to an array continuously
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
    description: signal to send to matching tabreceive~ object(s)
arguments:
  - type: symbol
    description: send symbol name 
    default: empty symbol
methods:
  - type: set <name>
    description: set table name
draft: false
---
By default a block is 64 samples but this can be changed with the block~ object.
