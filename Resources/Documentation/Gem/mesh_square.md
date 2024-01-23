---
title: mesh_square
description: renders a mesh in a square
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: resolution of the mesh
    default: 1
methods:
    - type: grid <float>
      description: change the grid resolution
    - type: gridX <float>
      description: change the X grid resolution
    - type: gridY <float>
      description: change the Y grid resolution
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