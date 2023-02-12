---
title: text size
description: get number of lines or elements
categories:
- object
pdcategory: vanilla, Data Management
last_update: '0.49'
see_also:
- list
- array
- scalar
- text define
- text get
- text set
- text insert
- text delete
- text
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- description: text name if no flags are given 
  default: none
  type: symbol
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
inlets:
  1st:
  - type: bang
    description: output the number of lines
  - type: float
    description: set line number and output its length
  2nd:
  - type: symbol
    description: set text name
  - type: pointer
    description: pointer to the text if -s flag is used
outlets:
  1st:
  - type: float
    description: number of lines or line length
draft: false
---
"text size" reports the number of lines in the text or the length of a specified line.
