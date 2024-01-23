---
title: pix_threshold_bernsen
description: apply dynamic thresholds to pixels for binarization
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: number of tiles in x-direction
    default: 16
  - type: float
    description: number of tiles in y-direction
    default: 16
methods:
  - type: float
    description: number of tiles
  - type: float
    description: contrast. if the (max-min)<contrast, values are clamped to 0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: number of tiles
  3rd:
    - type: float
      description: contrast
outlets:
  1st:
    - type: gemlist
draft: false
---
