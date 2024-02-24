---
title: pix_contrast
description: change contrast and saturation of an image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: contrast (>=0, default: 1)
    default: 1
  - type: float
    description: saturation (>=0, default: 1)
    default: 1
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: contrast (>=0, default: 1)
  3rd:
    - type: float
      description: saturation (>=0, default: 1)
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
