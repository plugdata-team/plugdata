
---
title: GEMglMaterialfv
description: specify material parameters for lighting with float values
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the face and the material parameter.
      default: GL_FRONT
    - type: float
      description: Specifies the material parameter.
      default: GL_DIFFUSE
    - type: float
      description: Specifies the RGBA parameter values.
      default: [0.2, 0.2, 0.2, 1.0]
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the face and the material parameter.
  3rd:
    - type: float
      description: Specifies the material parameter.
   4th:
    - type: list
      description: Specifies the RGBA parameter values.
outlets:
  1st:
    - type: gemlist
draft: false
---

