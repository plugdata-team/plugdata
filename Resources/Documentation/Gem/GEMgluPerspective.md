
---
title: GEMgluPerspective
description: set up a perspective projection matrix
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the field of view angle, in degrees, in the y direction. 
      default: 0
    - type: float
      description: Specifies the aspect ratio that determines the field of view in the x direction. The aspect ratio is the ratio of x (width) to y
      default: 0
    - type: float
      description: Specifies the distance from the viewer to the near clipping plane (always positive). 
      default: 0
    - type: float
      description: Specifies the distance from the viewer to the far clipping plane (always positive).    
      default: 0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the field of view angle, in degrees, in the y direction. 
  3rd:
    - type: float
      description: Specifies the aspect ratio that determines the field of view in the x direction. The aspect ratio is the ratio of x (width) to y
  4th:
    - type: float
      description: Specifies the distance from the viewer to the near clipping plane (always positive). 
  5th:
    - type: float
      description: Specifies the distance from the viewer to the far clipping plane (always positive). 
outlets:
  1st:
    - type: gemlist
draft: false


