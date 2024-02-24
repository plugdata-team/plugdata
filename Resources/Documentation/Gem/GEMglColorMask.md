
---
title: GEMglColorMask
description: enable and disable writing of frame buffer color components
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies whether red, green, blue, and alpha can be written into the frame buffer.
      default: [true, true, true, true]
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies whether red, green, blue, and alpha can be written into the frame buffer.
outlets:
  1st:
    - type: gemlist
draft: false
---

