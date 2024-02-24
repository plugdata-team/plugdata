
---
title: GEMglMateriali
description: specify material parameters for lighting with integer values
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
      description: Specifies the parameter value.
      default: 0
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
    - type: float
      description: Specifies the parameter value.
outlets:
  1st:
    - type: gemlist
draft: false
---

