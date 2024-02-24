
---
title: GEMglCopyTexImage1D
description: copy pixels into a 1D texture image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the target texture.
      default: GL_TEXTURE_1D
    - type: float
      description: Specifies the level-of-detail number. Level 0 is the base image level. Level n is the nth mipmap reduction image.
      default: 0
    - type: float
      description: Specifies the internal format of the texture image.
      default: GL_RGBA
    - type: float
      description: Specifies the xoffset parameter, the lower left corner of the texture image is (xoffset, 0).
      default: 0
    - type: float
      description: Specifies the width of the texture image.
      default: 0
    - type: float
      description: Specifies the border of the texture. Must be 0.
      default: 0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the target texture.
  3rd:
    - type: float
      description: Specifies the level-of-detail number. Level 0 is the base image level. Level n is the nth mipmap reduction image.
  4th:
    - type: float
      description: Specifies the internal format of the texture image.
  5th:
    - type: float
      description: Specifies the xoffset parameter, the lower left corner of the texture image is (xoffset, 0).
  6th:
    - type: float
      description: Specifies the width of the texture image.
  7th:
    - type: float
      description: Specifies the border of the texture. Must be 0.
outlets:
  1st:
    - type: gemlist
draft: false
---

