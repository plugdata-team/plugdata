---
title: tabwrite~
description: write a signal in an array
categories:
- object
see_also:
- tabread4~
- tabread
- tabwrite
- tabsend~
- tabreceive~
- soundfiler
pdcategory: vanilla, Arrays and Tables
last_update: '0.40'
arguments:
- type: symbol
  description: sets table name with the sample
inlets:
  1st:
  - type: signal
    description: signal to write to an array
  - type: bang
    description: starts recording into the array

methods:
  - type: start <float>
    description: starts recording at given sample
  - type: stop
    description: stops recording into the array
  - type: set <symbol>
    description: set the table name
draft: false
---
