---
title: text insert
description: insert a line
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
- text
- text delete
- text size
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- description: array name if no flags are given
  default: none
  type: symbol
- description: set message number
  default: 0
  type: float
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
inlets:
1st:
- type: list
  description: a line to insert
2nd:
- type: float
  description: message number to insert
3rd:
- type: symbol
  description: set text name
- type: pointer
  description: pointer to the text if -s flag is used
draft: false
---
