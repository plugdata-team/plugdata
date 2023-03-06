---
title: tabwrite
description: write a number to a table
categories:
- object
pdcategory: vanilla, Arrays and Tables
last_update: '0.33'
see_also:
- array
- tabread
- tabread4
- tabwrite~
arguments:
- description: sets table name with the sample
  type: symbol
inlets:
  1st:
  - type: float
    description: sets index to write to
  2nd:
  - type: float
    description: sets index to write to
methods:
  - type: set <symbol>
    description: set the table name
draft: false
---
Tabwrite writes floats into an array,  input values are set in the left inlet,  while the index is set on the right inlet.
