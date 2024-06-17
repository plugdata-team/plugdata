
---
title: GEMglMultiTexCoord2f
description: set the texture coordinates for a specified texture unit with two float values
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the texture unit.
      default: GL_TEXTURE0
    - type: float
      description: Specifies the s and t texture coordinates.
      default: 0.0
      min: 0.0
      max: 1.0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the texture unit.
  3rd:
    - type: float
      description: Specifies the s and t texture coordinates.
outlets:
  1st:
    - type: gemlist
draft: false
---

