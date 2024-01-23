
---
title: GEMglPolygonOffset
description: set the scale and units used to calculate depth values
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies a scale factor that is used to create a variable depth offset for each polygon.
      default: 0.0
    - type: float
      description: Specifies a constant factor that is used to create a variable depth offset for each polygon.
      default: 0.0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies a scale factor that is used to create a variable depth offset for each polygon.
  3rd:
    - type: float
      description: Specifies a constant factor that is used to create a variable depth offset for each polygon.
outlets:
  1st:
    - type: gemlist
draft: false
---

