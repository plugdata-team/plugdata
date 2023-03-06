---
title: text get
description: read and output a line
categories:
- object
pdcategory: vanilla, Data Management
last_update: '0.49'
see_also:
- list
- array
- scalar
- text define
- text
- text set
- text insert
- text delete
- text size
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- type: symbol
  description: text name if no flags are given 
  default: none
- type: float
  description: starting element number
  default: -1
- type: float
  description: initial number of elements
  default: 1
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
inlets:
1st:
- type: float
  description: specify message number and output (0 for first message)
2nd:
- type: float
  description: starting element number (-1 for the whole message
3rd:
- type: float
  description: specify number of elements
4th:
- type: symbol
  description: set text name
- type: pointer
  description: pointer to the text if -s flag is used
outlets:
1st:
- type: list
  description: a message from text or elements from it
2nd:
- type: float
  description: 0 if terminated by semicolon, 1 if by comma, or 2 if message number out of range
draft: false
---
