---
title: tabread
description: read a number from a table
categories:
- object
pdcategory: vanilla, Arrays and Tables
last_update: '0.43'
see_also:
- tabplay~
- tabread4
- tabreceive~
- tabsend~
- tabwrite
- tabwrite~
arguments:
- description: sets table name with the sample
  type: symbol
inlets:
  1st:
  - type: float
    description: sets table index and output its value
outlets:
  1st:
  - type: float
    description: value of index input

methods:
  - type: set <symbol>
    description: set the table name
draft: false
---
The tabread object reads values from an array ("table") according to an index. The index is rounded down to the next lower integer. Values in the table correspond to indices starting at 0 Indices outside of the range are replaced by the nearest index in range.

Check also the "array" examples from the Pd tutorial by clicking and opening `doc/2.control.examples/16.more.arrays`
