---
title: pix_movie
description: loads in a preproduced digital video to be used as a texture, bitblit, or something else
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: file to load initially
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: sets the frame number to output
outlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: the dimensions (in frames and pixels) of a film when it gets loaded
  3rd:
    - type: bang
      description: indicates that the last frame has been reached

draft: true
---
