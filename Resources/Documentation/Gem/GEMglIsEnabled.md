
---
title: GEMglIsEnabled
description: test whether a specific GL capability is enabled
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the capability to test.
      default: GL_LIGHTING
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the capability to test.
outlets:
  1st:
    - type: float
      description: Returns GL_TRUE if the capability is enabled, and GL_FALSE otherwise.
draft: false
---

