
---
title: GEMglGetMapfv
description: return the coefficients of the specified map
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the map to query.
      default: GL_COEFF
    - type: float
      description: Specifies the target coordinate space.
      default: GL_OBJECT_LINEAR
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the map to query.
  3rd:
    - type: float
      description: Specifies the target coordinate space.
  4th:
    - type: float
      description: Returns the coefficients of the specified map.
outlets:
  1st:
    - type: gemlist
draft: false
---

