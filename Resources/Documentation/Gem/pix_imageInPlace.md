---
title: pix_imageInPlace
description: loads in multiple image files
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: filename (with wildcard *) for images to load
methods:
  - type: preload
    description: open images (the wildcard in the filename is expanded with integer 0..#)
  - type: download
    description: load the "preload"ed images into texture-RAM
  - type: purge
    description: delete images from texture-RAM
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
