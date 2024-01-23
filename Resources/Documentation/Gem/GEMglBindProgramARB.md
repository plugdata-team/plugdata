
---
title: GEMglBindProgramARB
description: bind a program object (ARB extension)
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the program target (e.g., GL_VERTEX_PROGRAM_ARB).
      default: 0
    - type: float
      description: Specifies the name of the program object to bind.
      default: 0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the program target (e.g., GL_VERTEX_PROGRAM_ARB).
  3rd:
    - type: float
      description: Specifies the name of the program object to bind.
outlets:
  1st:
    - type: gemlist
draft: false
---

