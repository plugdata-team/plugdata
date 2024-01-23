
---
title: GEMglDrawArrays
description: render primitives from array data
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies what kind of primitives to render.
      default: GL_TRIANGLES
    - type: float
      description: Specifies the starting index in the enabled arrays.
      default: 0
    - type: float
      description: Specifies the number of indices to be rendered.
      default: 0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies what kind of primitives to render.
  3rd:
    - type: float
      description: Specifies the starting index in the enabled arrays.
  4th:
    - type: float
      description: Specifies the number of indices to be rendered.
outlets:
  1st:
    - type: gemlist
draft: false
---

