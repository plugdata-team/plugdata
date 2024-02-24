---
title: pix_crop
description: get a subimage of an image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - name: offsetX
    type: float
    description: offset from the left edge of the image
  - name: offsetY
    type: float
    description: offset from the bottom edge of the image
  - name: dimenX
    type: float
    description: width of the subimage in pixels
  - name: dimenY
    type: float
    description: height of the subimage in pixels
inlets:
  1st:
    - type: gemlist
  2nd:
    - type: float
      description: width
  3rd:
    - type: float
      description: height
  4th:
    - type: float
      description: offset X
  5th:
    - type: float
      description: offset Y
outlets:
  1st:
    - type: gemlist
draft: false
---
