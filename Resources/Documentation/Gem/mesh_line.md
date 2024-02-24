---
title: mesh_line
description: renders a mesh in a line
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: draw <symbol>
    description: line, default or point
    
arguments:
  - type: float
    description: resolution of the mesh
    default: 1

inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: size
outlets:
  1st:
    - type: gemlist

draft: false
---
