---
title: pix_snap
description: take a screenshot and convert it to a pix
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: x offset, y offset, width, height
methods:
    - type: snap
      description: do grab
    - type: type <symbol>
      description: set type (BYTE|FLOAT|DOUBLE)
    - type: dimen <float> <float.
      description: set size (width, height)
    - type: offset <float> <float>
      description: set offset (x, y)
inlets:
  1st:
    - type: gemlist
      description:
    - type: bang
      description: do grab
  2nd:
    - type: list
      description: x offset, y offset
  3rd:
    - type: list
      description: width, height
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
