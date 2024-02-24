
---
title: GEMglLighti
description: set the value of a light source parameter
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the light source.
      default: GL_LIGHT0
    - type: float
      description: Specifies the light source parameter.
      default: GL_SPOT_EXPONENT
    - type: float
      description: Specifies the parameter value.
      default: 0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the light source.
  3rd:
    - type: float
      description: Specifies the light source parameter.
  4th:
    - type: float
      description: Specifies the parameter value.
outlets:
  1st:
    - type: gemlist
draft: false
---

