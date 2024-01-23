---
title: pix_chroma_key
description: mix 2 images based on their color
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: float
    description: which stream is the key-source (0=left stream, 1=right stream)
  - type: list
    description: list of 3 floats of the pixel-value to key out
  - type: list
    description: list of 3 floats defining the +/-range of the key
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
