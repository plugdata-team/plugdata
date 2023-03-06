---
title: plot
description: draw array elements of scalars
categories:
- object
see_also: 
- drawnumber
- drawpolygon
- struct
- array
- element
pdcategory: vanilla, Data Structures
last_update: '0.52'
inlets:
  1st:
  - type: float
    description: sets visibility (1 = visible, 0 = invisible)
flags:
- name: -c
  description: sets to Bezier curve plot
- name: -n 
  description: makes it invisible initially
- name: -v <float/symbol>
  description: constant of field name to make it visible/invisible
- name: -vs <float/symbol>
  description: constant of field name to scalars visible/invisible
- name: -e <float/symbol
  description: constant of field name to enable/disable mouse actions
- name: -x <symbol>
  description: sets a different field name for x
- name: -y <symbol>
  description: sets a different field name for y
- name: -w <symbol>
  description: sets a different field name for w
arguments:
- type: symbol
  description: (optional) 'curve' sets to Bezier curve 
  default: polygon
- type: symbol
  description: array field name to plot
- type: float
  description: color
  default: 0
- type: float
  description: line width
  default: 1
- type: list
  description: relative x and y location
  default: 0 0
- type: float
  description: x spacing
draft: false
---
