
---
title: GEMglClipPlane
description: specify a plane against which all geometry is clipped
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies which clipping plane is being specified.
      default: 0
    - type: float
      description: Specifies the equation coefficients of the clipping plane.
      default: [0, 0, 0, 0]
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies which clipping plane is being specified.
  3rd:
    - type: float
      description: Specifies the equation coefficients of the clipping plane.
outlets:
  1st:
    - type: gemlist
draft: false


