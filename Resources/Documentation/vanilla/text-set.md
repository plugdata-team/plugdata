---
title: text set
description: replace or add a line
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
- text
- text insert
- text delete
- text size
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- description: text name if no flags are given 
  default: none
  type: symbol
- description: set message number
  default: 0
  type: float
- description: set element number
  default: -1
  type: float
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
inlets:
1st:
- type: list
  description: a message to add or replace
2nd:
- type: float
  description: message number to replace or add (if greater than the number of messages)
3rd:
- type: float
  description: element number to start replacing, or replace whole message if negative
4th:
- type: symbol
  description: set text name
- type: pointer
  description: pointer to the text if -s flag is used
outlets:
draft: false
---
