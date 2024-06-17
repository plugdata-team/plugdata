
---
title: GEMglCopyPixels
description: copy pixels into a 1D or 2D texture image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the left x-coordinate, in pixels, of the source rectangle.
      default: 0
    - type: float
      description: Specifies the lower y-coordinate, in pixels, of the source rectangle.
      default: 0
    - type: float
      description: Specifies the width of the texture image.
      default: 0
    - type: float
      description: Specifies the height of the texture image.
      default: 0
    - type: float
      description: Specifies the format of the pixel data.
      default: GL_COLOR
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the left x-coordinate, in pixels, of the source rectangle.
  3rd:
    - type: float
      description: Specifies the lower y-coordinate, in pixels, of the source rectangle.
  4th:
    - type: float
      description: Specifies the width of the texture image.
  5th:
    - type: float
      description: Specifies the height of the texture image.
  6th:
    - type: float
      description: Specifies the format of the pixel data.
outlets:
  1st:
    - type: gemlist
draft: false


