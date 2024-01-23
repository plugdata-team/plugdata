---
title: pix_dot
description: simplify an image by representing each segment with a white dot whose size is relative to the luminance of the original segment
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: size of the dots
inlets:
  1st:
    - type: gemlist
      description: gemlist
  2nd:
    - type: float
      description: size of the dots
outlets:
  1st:
    - type: gemlist
draft: false
---
