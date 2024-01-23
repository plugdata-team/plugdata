---
title: pix_buffer
description: a storage place for a number of images
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: allocate <float> <float>
    description: preallocate memory for images
  - type: open <symbol> <float>
    description: put an image into the pix_buffer at the given index
  - type: resize <float>
    description: re-allocate slots in the buffer (slots will survive this)
  - type: copy <symbol> <symbol>
    description: copy a pix from one slot to another
  - type: save <symbol> <float>
    description: save image in a given slot to hard disk
inlets:
  1st:
    - type: bang
      description: get the size of the buffer in frames
outlets:
  1st:
    - type: float
      description: size of the buffer
draft: false
---
