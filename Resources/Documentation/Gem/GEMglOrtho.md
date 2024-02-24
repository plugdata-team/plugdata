
---
title: GEMglOrtho
description: multiply the current matrix with an orthographic matrix
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the coordinates for the left and right vertical clipping planes.
      default: 0.0
    - type: float
      description: Specifies the coordinates for the bottom and top horizontal clipping planes.
      default: 0.0
    - type: float
      description: Specifies the coordinates for the near and far depth clipping planes.
      default: 0.0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the coordinates for the left and right vertical clipping planes.
  3rd:
    - type: float
      description: Specifies the coordinates for the bottom and top horizontal clipping planes.
  4th:
    - type: float
      description: Specifies the coordinates for the near and far depth clipping planes.
outlets:
  1st:
    - type: gemlist
draft: false
---

