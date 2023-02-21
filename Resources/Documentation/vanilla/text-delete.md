---
title: text delete
description: delete a line or clear
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
- text
- text size
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- description: array name if no flags are given 
  default: none
  type: symbol
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure

inlets:
  1st:
  - type: float
    description: line number to delete (negative deletes all text)
  2nd:
  - type: symbol
    description: set text name
  - type: pointer
    description: pointer to the text if -s flag is used
draft: false
---
