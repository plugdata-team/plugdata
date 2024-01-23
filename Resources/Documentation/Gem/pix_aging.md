---
title: pix_aging
description: apply a super8-like aging effect
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: scratch <float>
    description: add a maximum of scratches
  - type: coloraging <float>
    description: color-bleaching
  - type: dust <float>
    description: add "dust"
  - type: pits <float>
    description: add "pits"
inlets:
  1st:
    - type: gemlist
    - type: float
      description: add a maximum of scratches
    - type: float
      description: color-bleaching
    - type: float
      description: add "dust"
    - type: float
      description: add "pits"
outlets:
  1st:
    - type: gemlist
draft: false
---
