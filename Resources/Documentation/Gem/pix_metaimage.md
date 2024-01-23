---
title: pix_metaimage
description: display a pix by itself
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: size
methods:
    - type: cheap <float>
      description: use cheap algorithm 
    - type:  distance <float>
      description: use distance-based algorithm
inlets:
  1st:
    - type: gemlist
      description:
    - type: float
      description: apply/don't apply
  2nd:
    - type: float
      description: size
outlets:
  1st:
    - type: gemlist
      description:

draft: false
---
