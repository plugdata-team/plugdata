
---
title: GEMglNewList
description: create or replace a display list
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies a unique integer that represents the display list.
      default: 1
    - type: float
      description: Specifies the compilation mode.
      default: GL_COMPILE
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies a unique integer that represents the display list.
  3rd:
    - type: float
      description: Specifies the compilation mode.
outlets:
  1st:
    - type: gemlist
draft: false
---


