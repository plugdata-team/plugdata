---
title: pix_share_write
description: write pixels to a shared memory region
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: ID
  - type: float
    description: width
  - type: float
    description: height
  - type: symbol
    description: colourspace
methods:
  - type: set <float> <float> <float> <symbol>
    description:
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: error number
draft: false
---
