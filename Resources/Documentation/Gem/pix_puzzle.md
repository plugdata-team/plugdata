---
title: pix_puzzle
description: shuffle an image
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: size <float> <float>
    description: number of elements in x/y
  - type: move <float>
    description: move the empty field
inlets:
  1st:
    - type: gemlist
      description:
    - type: float
      description: apply/don't apply
    - type: bang
      description: reshuffle
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
