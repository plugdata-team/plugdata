---
title: pix_mix
description: mix 2 images based on mixing factors
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: left gain
    default: 0.5
  - type: float
    description: right gain
    default: 0.5

inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: gemlist
      description:
  3rd:
    - type: list
      description: weights for left/right image
    - type: float
      description: weight for left image, right weight will be the reciprocal value
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
