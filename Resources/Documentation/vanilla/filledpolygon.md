---
title: filledpolygon
description: draw shapes for data structures
categories:
- object
see_also: 
- struct
- drawtext
- plot
- set
- text
pdcategory: vanilla, Data Structures
last_update: '0.51'
inlets:
  1st:
  - type: float
    description: sets visibility (1 = visible, 0 = invisible)
flags:
- name: -n
  description: makes it invisible initially
- name:  -v <symbol>
  description: assigns a field to make it visible/invisible
- name: -xr
  description: disable mouse actions when in "run" mode
- name: -xe
  description: disable mouse actions when in "edit" mode
- name: -x
  description: set both "-xr" and "-xe", disabling all mouse actions
- name: -xv
  description: disable dragging vertices when in "run" mode (keeps reporting mouse clicks)
arguments:
- type: float
  description: line color, filled shapes have an extra first interior color
  default: 0, 0
- type: float
  description: line width 
  default: 1
- type: list
  description: (x, y) pairs giving coordinates of each vertex
draft: false
---
