---
title: pix_snap2tex
description: take a screenshot and texture it
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: X offset
  - type: float
    description: Y offset
  - type: float
    description: width
  - type: float
    description: height
methods:
    - type: quality <float>
      description: turn texture-resampling on/off
inlets:
  1st:
    - type: gemlist
      description:
    - type: bang
      description: do grab!
    - type: float
      description: turn texturing on/off
  2nd:
    - type: list
      description: X and Y offset
  3rd:
    - type: list
      description: width adn height
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
