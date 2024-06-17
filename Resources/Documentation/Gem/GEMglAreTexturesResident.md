
---
title: GEMglAreTexturesResident
description: determine if textures are loaded in texture memory
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the number of texture names to be queried.
      default: 0
    - type: float
      description: Specifies an array containing the texture names to be queried.
      default: []
    - type: float
      description: Returns an array indicating whether each corresponding texture is resident.
      default: []
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the number of texture names to be queried.
  3rd:
    - type: float
      description: Specifies an array containing the texture names to be queried.
outlets:
  1st:
    - type: gemlist
      description: Returns an array indicating whether each corresponding texture is resident.
draft: false

