
---
title: GEMglEvalMesh1
description: compute a one-dimensional mesh
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the mesh type.
      default: GL_LINE
    - type: float
      description: Specifies the starting grid coordinate.
      default: 0
    - type: float
      description: Specifies the number of steps in the grid.
      default: 1
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the mesh type.
  3rd:
    - type: float
      description: Specifies the starting grid coordinate.
  4th:
    - type: float
      description: Specifies the number of steps in the grid.
outlets:
  1st:
    - type: gemlist
draft: false
---

