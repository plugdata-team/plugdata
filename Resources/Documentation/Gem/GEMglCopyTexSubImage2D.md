
---
title: GEMglCopyTexSubImage2D
description: copy a two-dimensional texture subimage
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the target texture.
      default: GL_TEXTURE_2D
    - type: float
      description: Specifies the level-of-detail number. Level 0 is the base image level. Level n is the nth mipmap reduction image.
      default: 0
    - type: float
      description: Specifies the xoffset and yoffset parameters. The lower left corner of the texture image is (xoffset, yoffset).
      default: [0, 0]
    - type: float
      description: Specifies the x, y, width, and height parameters, which specify a subregion of the framebuffer.
      default: [0, 0, 0, 0]
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
      description: Specifies the xoffset and yoffset parameters. The lower left corner of the texture image is (xoffset, yoffset).
  5th:
    - type: float
      description: Specifies the x, y, width, and height parameters, which specify a subregion of the framebuffer.
outlets:
  1st:
    - type: gemlist
draft: false
---

