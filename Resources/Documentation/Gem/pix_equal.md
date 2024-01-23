---
title: pix_equal
description: marks the pixels nearly equal to a given color
categories:
  - object
pdcategory: Gem, Graphics
inlets:
  1st:
    - type: gemlist
      description: gemlist (RGBA image)
  2nd:
    - type: list
      description: list of lower bounds [R G B A=1], ranging from 0 to 1
  3rd:
    - type: list
      description: list of upper bounds [R G B A=1], ranging from 0 to 1
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
