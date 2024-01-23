
---
title: GEMglCopyTexSubImage1D
description: copy a one-dimensional texture subimage
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
      description: Specifies the xoffset parameter. The lower left corner of the texture image is (xoffset, 0).
      default: 0
    - type: float
      description: Specifies the x, y, and width parameters, which specify a subregion of the framebuffer.
      default: [0, 0, 0]
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
      description: Specifies the xoffset parameter. The lower left corner of the texture image is (xoffset, 0).
  5th:
    - type: float
      description: Specifies the x, y, and width parameters, which specify a subregion of the framebuffer.
outlets:
  1st:
    - type: gemlist
draft: false
---

