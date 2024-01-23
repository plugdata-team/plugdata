---
title: pix_buffer_read
description: reads an image from the named buffer provided by [pix_buffer]
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: <buffer_name>
methods:
  - type: set <symbol>
    description: read from another buffer
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: index of the frame in the named pix_buffer to read
outlets:
  1st:
    - type: gemlist
draft: false
---
