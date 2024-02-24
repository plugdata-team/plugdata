---
title: pix_image
description: loads in an image to be used as texture, bitblit, or something else
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: filename
methods:
  - type: open
    description: open image file with specified filename
  - type: thread
    description: specify whether to use a separate thread for image loading
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
