---
title: drawtext
description: draw numbers, symbols or texts for data structures
categories:
- object
see_also: 
- struct
- drawpolygon
- plot
- set
pdcategory: vanilla, Data Structures
last_update: '0.50'
inlets:
  1st:
  - type: float
    description: sets visibility (1 = visible, 0 = invisible)
flags:
- name: -n
  description: makes it invisible initially
- name:  -v <symbol>
  description: assigns a field to make it visible/invisible
arguments:
- type: symbol
  description: field name to draw
- type: list
  description: (x, y) coordinates pair 
  default: 0 0
- type: float
  description: color 
  default: 0
- type: symbol
  description: optional label
draft: false
---
