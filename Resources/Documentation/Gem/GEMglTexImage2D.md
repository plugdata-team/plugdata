
---
title: GEMglTexImage2D
description: specify a two-dimensional texture image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the target texture.
    - type: float
      description: Specifies the level-of-detail number. Level 0 is the base image level.
    - type: float
      description: Specifies the internal format of the texture.
    - type: float
      description: Specifies the width of the texture image.
    - type: float
      description: Specifies the height of the texture image.
    - type: float
      description: Specifies the border of the texture.
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
      description: Specifies the level-of-detail number. Level 0 is the base image level.
  4th:
    - type: float
      description: Specifies the internal format of the texture.
  5th:
    - type: float
      description: Specifies the width of the texture image.
  6th:
    - type: float
      description: Specifies the height of the texture image.
  7th:
    - type: float
      description: Specifies the border of the texture.
  8th:
    - type: float
      description: Specifies the format of the pixel data.
  9th:
    - type: float
      description: Specifies the data type of the pixel data.
  10th:
    - type: const GLvoid*
      description: Specifies a pointer to the image data in memory.
outlets:
  1st:
    - type: gemlist
draft: false
---

