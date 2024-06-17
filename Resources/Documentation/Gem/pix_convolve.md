---
title: pix_convolve
description: apply a convolution kernel to an image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: matrix dimensions
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: scale factor
    - type: list
      description: the convolution kernel
outlets:
  1st:
    - type: gemlist
draft: false
---
