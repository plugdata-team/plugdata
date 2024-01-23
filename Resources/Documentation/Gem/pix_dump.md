---
title: pix_dump
description: dump all the pixel-data of an image
categories:
  - object
pdcategory: Graphics
methods:
- type: bytemode <float>
    description: set normalization on or off
inlets:
  1st:
    - type: gemlist
      description: gemlist
    - type: bang
      description: do dump

outlets:
  1st:
    - type: gemlist
  2nd:
    - type: list
      description: dumped pixel data
draft: false
---
