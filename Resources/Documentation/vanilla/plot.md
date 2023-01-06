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
pdcategory: Data Templates
last_update: '0.52'
inlets:
  1st:
  - type: float
    description: "sets visibility (1: visible, 0: invisible)."
flags:
- flag: -c
  description: sets to Bezier curve plot.
- flag: -n 
  description: makes it invisible initially.
- flag: -v <float/symbol>
  description: constant of field name to make it visible/invisible.
- flag: -vs <float/symbol>
  description: constant of field name to scalars visible/invisible.
- flag: -e <float/symbol
  description: constant of field name to enable/disable mouse actions.
- flag: -x <symbol>
  description: sets a different field name for x.
- flag: -y <symbol>
  description: sets a different field name for y.
- flag: -w <symbol>
  description: sets a different field name for w.
arguments:
- type: symbol
  description: (optional) 'curve' sets to Bezier curve 
  default: polygon
- type: symbol
  description: field to plot the array.
- type: float*
  description: color 