---
title: pix_data
description: gets the color of a specified pixel within an image when triggered
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: xpos
    - type: float
      description: ypos
methods:
  - type: normalize <float>
    description: normalize 0|1: normalization mode (0=none, 1=normalized. default=1)
  - type: quality <float>
    description: quality 0|1: interpolation mode (0=none, 1=2D. default=0)
inlets:
  1st:
    - type: bang
      description:
    - type: list <float> <float>
      description:
  2nd:
    - type: gemlist
      description:
  3rd:
    - type: float
      description: xpos in the image
  4th:
    - type: float
      description: ypos in the image
outlets:
  1st:
    - type: -
      description: dead outlet for legacy reasons
  2nd:
    - type: list
      description: RGB value (normalized 0..1)
  3rd:
    - type: float
      description: grey value (normalized 0..1)
draft: false
---
