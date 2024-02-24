---
title: pix_rds
description: random dot stereogram for luminance
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: stride distance
    default: 40
methods:
    - type: method <float>
      description: crosseyed (0) or walleyed (1)
    - type: stride <float>
      description: stride distance
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
