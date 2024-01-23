---
title: pix_multiimage
description: loads in an image to be used as a texture, bitblit, or something else.
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: filename (with wildcard *) for images to load
methods:
    - type: open <symbol> <float>
      description: open images
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: select image (starting at 0)
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
