---
title: torus

description: renders a torus

categories:
 - graphics

pdcategory: Gem, Graphics

arguments:
- type: float
  description: size of the torus
  default: 1
- type: float
  description: number of slices
  default: 14
- type: float
  description: thickness
  default: 1

methods:
- type: draw <symbol>
  description: 'fill', 'line' or 'points'
  
inlets:
  1st:
  - type: gemlist
    description:
  2nd:
  - type: float
    description: torus size
  3rd:
  - type: float
    description: number of slices
  4th:
  - type: float
    description: torus thickness

outlets:
  1st:
  - type: gemlist
    description:

draft: false
---