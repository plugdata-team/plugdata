
---
title: GEMglMap1d
description: define a one-dimensional evaluator
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the kind of values being mapped.
      default: GL_MAP1_VERTEX_3
    - type: float
      description: Specifies the coefficients for the map.
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the kind of values being mapped.
  3rd:
    - type: float
      description: Specifies the coefficients for the map.
outlets:
  1st:
    - type: gemlist
draft: false
---

