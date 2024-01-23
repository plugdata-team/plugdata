---
title: teapot

description: renders a teapot

categories:
 - graphics

pdcategory: Gem, Graphics

arguments:
- type: float
  description: size of the teapot
  default: 1
- type: float
  description: number of slices
  default: 14

methods:
- type: draw <symbol>
  description: 'fill', 'line' or 'points'
  
inlets:
  1st:
  - type: gemlist
    description:
  2nd:
  - type: float
    description: teapot size
  3rd:
  - type: float
    description: number of slices

outlets:
  1st:
  - type: gemlist
    description:

draft: false
---