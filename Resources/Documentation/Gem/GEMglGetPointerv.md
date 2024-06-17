
---
title: GEMglGetPointerv
description: return the address of the specified pointer
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the pointer to be returned.
      default: GL_VERTEX_ARRAY_POINTER
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the pointer to be returned.
  3rd:
    - type: gemlist
      description: Returns the address of the specified pointer.
outlets:
  1st:
    - type: gemlist
draft: false
---

