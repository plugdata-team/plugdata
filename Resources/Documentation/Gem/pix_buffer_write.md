---
title: pix_buffer_write
description: writes an image into a named buffer created by the [pix_buffer] object
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: <buffer_name>
methods:
  - type: set <symbol>
    description: write to another buffer
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: index in the named pix_buffer
outlets:
  1st:
    - type: gemlist
draft: false
---
