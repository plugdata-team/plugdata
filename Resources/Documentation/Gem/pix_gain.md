---
title: pix_gain
description: multiply pixel-values
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: <list>
    description: gain vector
inlets:
  1st:
    - type: gemlist
      description:
    - type: <float>
      description: multiplier for all channels
  2nd:
    - type: list
      description: 3 (RGB) or 4 (RGBA) values as multipliers for each channel
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
