
---
title: GEMglMapGrid2f
description: define a two-dimensional grid for evaluators
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the number of partitions in the u direction of the grid.
      default: 1
    - type: float
      description: Specifies the coordinates of the two end points of the u grid.
    - type: float
      description: Specifies the number of partitions in the v direction of the grid.
      default: 1
    - type: float
      description: Specifies the coordinates of the two end points of the v grid.
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the number of partitions in the u direction of the grid.
  3rd:
    - type: float
      description: Specifies the coordinates of the two end points of the u grid.
  4th:
    - type: float
      description: Specifies the number of partitions in the v direction of the grid.
  5th:
    - type: float
      description: Specifies the coordinates of the two end points of the v grid.
outlets:
  1st:
    - type: gemlist
draft: false
---

