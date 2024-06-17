
---
title: GEMglIsList
description: test if a name corresponds to a display list
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the name of a display list.
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the name of a display list.
outlets:
  1st:
    - type: float
      description: Returns GL_TRUE if the name corresponds to a display list, and GL_FALSE otherwise.
draft: false
---

