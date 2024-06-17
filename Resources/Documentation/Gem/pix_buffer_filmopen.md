---
title: pix_buffer_filmopen
description: reads a movie into a named buffer in the pix_buffer object
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: buffer name
inlets:
  1st:
  - type: open <symbol> <float>
    description: read a filename into buffer starting at index
  - type: set <symbol>
    description: write to another buffer
outlets:
  1st:
    - type: list
      description: <length> <width> <height>
  2nd:
    - type: bang
      description: bangs when finished loading
draft: false
---
