---
title: pix_threshold
description: apply a threshold to pixels
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: threshold (scalar/vector)
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: threshold for all channels
  3rd:
    - type: list
      description: threshold (RGB) or (RGBA)
outlets:
  1st:
    - type: gemlist
draft: false
---
