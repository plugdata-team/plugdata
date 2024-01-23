
---
title: GEMglTexSubImage1D
description: define a subregion of a one-dimensional texture image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the target texture.
    - type: float
      description: Specifies the level-of-detail number.
    - type: float
      description: Specifies the xoffset of the subregion.
    - type: float
      description: Specifies the width of the subregion.
    - type: float
      description: Specifies the format of the pixel data.
    - type: float
      description: Specifies the data type of the pixel data.
    - type: const GLvoid*
      description: Specifies a pointer to the image data in memory.
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the target texture.
  3rd:
    - type: float
      description: Specifies the level-of-detail number.
  4th:
    - type: float
      description: Specifies the xoffset of the subregion.
  5th:
    - type: float
      description: Specifies the width of the subregion.
  6th:
    - type: float
      description: Specifies the format of the pixel data.
  7th:
    - type: float
      description: Specifies the data type of the pixel data.
  8th:
    - type: const GLvoid*
      description: Specifies a pointer to the image data in memory.
outlets:
  1st:
    - type: gemlist
draft: false
---

