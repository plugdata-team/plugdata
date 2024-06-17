
---
title: GEMglBitmap
description: draw a bitmap
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the width of the bitmap image.
      default: 0
    - type: float
      description: Specifies the height of the bitmap image.
      default: 0
    - type: float
      description: Specifies the x and y offsets from the current raster position.
      default: [0, 0]
    - type: float
      description: Specifies the x and y scales applied to the bitmap.
      default: [1, 1]
    - type: float
      description: Specifies the left and right boundary values for the bitmap.
      default: [0, 0]
    - type: float
      description: Specifies the minimum and maximum values of the map function.
      default: [0, 0]
    - type: float*
      description: Specifies a pointer to the bitmap image.
      default: null
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the width of the bitmap image.
  3rd:
    - type: float
      description: Specifies the height of the bitmap image.
  4th:
    - type: float
      description: Specifies the x and y offsets from the current raster position.
  5th:
    - type: float
      description: Specifies the x and y scales applied to the bitmap.
  6th:
    - type: float
      description: Specifies the left and right boundary values for the bitmap.
  7th:
    - type: float
      description: Specifies the minimum and maximum values of the map function.
  8th:
    - type: float*
      description: Specifies a pointer to the bitmap image.
outlets:
  1st:
    - type: gemlist
draft: false

