---
title: text fromlist
description: convert from list
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
- text size
- text tolist
- text
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
  - type: list
    description: sets contents of text from given list
  2nd:
  - type: symbol
    description: set text name
  - type: pointer
    description: pointer to the text if -s flag is used
draft: false
---
"text fromlist" converts a list such as "text tolist" would output and fills the text with it. Whatever the text had previously contained is discarded.
