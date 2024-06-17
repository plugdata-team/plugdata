
---
title: GEMglColorMaterial
description: specify the material parameters for the lighting model
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies which material face or faces are being updated.
      default: GL_FRONT_AND_BACK
    - type: float
      description: Specifies which of the ambient, diffuse, specular, and emission material parameters are being set.
      default: GL_AMBIENT_AND_DIFFUSE
    - type: float
      description: If GL_TRUE, the material parameter is set to the corresponding current color. If GL_FALSE, the corresponding material parameter is set to the value specified by params.
      default: GL_FALSE
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies which material face or faces are being updated.
  3rd:
    - type: float
      description: Specifies which of the ambient, diffuse, specular, and emission material parameters are being set.
  4th:
    - type: float
      description: If GL_TRUE, the material parameter is set to the corresponding current color. If GL_FALSE, the corresponding material parameter is set to the value specified by params.
outlets:
  1st:
    - type: gemlist
draft: false
---

