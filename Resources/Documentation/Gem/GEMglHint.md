
---
title: GEMglHint
description: specify implementation-specific hints
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the target.
      default: GL_PERSPECTIVE_CORRECTION_HINT
    - type: float
      description: Specifies the mode.
      default: GL_FASTEST
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the target.
  3rd:
    - type: float
      description: Specifies the mode.
outlets:
  1st:
    - type: gemlist
draft: false
---

