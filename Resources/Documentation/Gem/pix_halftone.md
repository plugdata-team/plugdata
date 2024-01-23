---
title: pix_halftone
description: draws the input using halftone patterns
categories:
  - object
pdcategory: Gem, Graphics
methods:
    - type: style <float>
      description: selects a style
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: pattern size (1-32)
  3rd:
    - type: float
      description: orientation in degrees (0-360)
  4th:
    - type: float
      description: smoothness (0-1)
outlets:
  1st:
    - type: gemlist
draft: false
---
