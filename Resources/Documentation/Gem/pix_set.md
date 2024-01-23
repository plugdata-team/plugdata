---
title: pix_set
description: set the pixel-data of an image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: width
  - type: float
    description: height
methods:
  - type: RGB
    description: set colourspace
  - type: RGBA
    description: set colourspace
  - type: GREY
    description: set colourspace
  - type: fill <list>
    description: set the whole image with value (value could be G, RGB or RGBA)
  - type: set <float> <float>
    description: set the size of the image    
inlets:
  1st:
    - type: gemlist
      description:
    - type: bang
      description: send image (to update texture, for example)
  2nd:
    - type: list
      description: interleaved image-data (R1 G1 B1 A1 R2 B2...) or (R1 B1 G1 R2 B2...) or (L1 L2 L3...)
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
