---
title: curve3d
description: renders a 3D bezier curve
categories:
  - object
pdcategory: Gem, Graphics
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: not used
outlets:
  1st:
    - type: gemlist
methods:
  - type: draw <symbol>
    description: line, fill or point
  - type: grid <float> <float>
    description: set grid size (X, Y are 2 int)
  - type: res <float> <float>
    description: set resolution (X, Y are 2 int)
  - type: set <float> <float> <float> <float> <float>
    description: set control point position (Mx,My = position in the matrix; X,Y,Z = position of the control point)
draft: false
---
