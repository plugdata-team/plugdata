
---
title: GEMglIndexMask
description: set the write enable mask for the color index buffers
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies a bit mask to enable and disable writing of individual bits in the color index buffers.
      default: 0xFFFFFFFF
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies a bit mask to enable and disable writing of individual bits in the color index buffers.
outlets:
  1st:
    - type: gemlist
draft: false
---

