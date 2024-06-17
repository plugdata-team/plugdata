
---
title: GEMglGetIntegerv
description: return the value or values of a selected parameter
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the parameter value to retrieve.
      default: GL_VIEWPORT
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the parameter value to retrieve.
  3rd:
    - type: float
      description: Returns the value or values of the specified parameter.
outlets:
  1st:
    - type: gemlist
draft: false
---

