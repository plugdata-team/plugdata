
---
title: GEMglDrawElements
description: render primitives from array data
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies what kind of primitives to render.
      default: GL_TRIANGLES
    - type: float
      description: Specifies the number of indices to be rendered.
      default: 0
    - type: float
      description: Specifies the type of the values in indices.
      default: GL_UNSIGNED_INT
    - type: float
      description: Specifies an offset in the array buffer bound to GL_ELEMENT_ARRAY_BUFFER.
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
      description: Specifies the number of indices to be rendered.
  4th:
    - type: float
      description: Specifies the type of the values in indices.
  5th:
    - type: float
      description: Specifies an offset in the array buffer bound to GL_ELEMENT_ARRAY_BUFFER.
outlets:
  1st:
    - type: gemlist
draft: false
---

