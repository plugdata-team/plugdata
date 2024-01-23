
---
title: GEMglDepthRange
description: specify mapping of depth values from normalized device coordinates to window coordinates
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the mapping of the near clipping plane to window coordinates.
      default: 0
    - type: float
      description: Specifies the mapping of the far clipping plane to window coordinates.
      default: 1
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the mapping of the near clipping plane to window coordinates.
  3rd:
    - type: float
      description: Specifies the mapping of the far clipping plane to window coordinates.
outlets:
  1st:
    - type: gemlist
draft: false
---

