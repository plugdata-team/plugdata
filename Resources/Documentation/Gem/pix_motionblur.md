---
title: pix_motionblur
description: apply motion blurring on a series of images
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: blur-factor (0=no blurring, 1=only blurring)

inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: blur-factor (0..no blurring, 1..only blurring)
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
