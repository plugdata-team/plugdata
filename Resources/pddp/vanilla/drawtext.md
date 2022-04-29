---
title: drawtext
description: draw numbers, symbols or texts for Data Structures.
categories:
- object
see_also: 
- struct
- drawpolygon
- plot
- set
pdcategory: Data Templates
last_update: '0.50'
inlets:
  1st:
  - type: float
    description: "sets visibility (1: visible, 0: invisible)."
flags:
- flag: -n
  description: makes it invisible initially.
- flag:  -v <symbol>
  description: assigns a field to make it visible/invisible.
arguments:
- type: symbol
  description: field name to draw.
- type: list*
  description: (x, y) coordinates pair (default 0 0).
- type: float*
  description: color (default 0).
- type: symbol
  description: optional label.
- type: '*'
  description: position and color can also be field names.
draft: false
---
`[drawnumber]`

`[drawsymbol]`

`[drawtext]`

All these objects are actually aliases of the same class. There's no difference in them. Use them to draw either numbers, symbols or texts fields.