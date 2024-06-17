---
title: pix_noise
description: generate a noise image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: width
  - type: float
    description: height
methods:
    - type: bang
      description: generate new image
    - type: auto <float>
      description: generate new image for each frame on/off
    - type: set <list>
      description: change size of the image
    - type: seed <float>
      description: change the seed of the random function\
    - type: RGB
      description: set colourspace
    - type: RGBA
      description: set colourspace
    - type: GREY
      description: set colourspace
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
