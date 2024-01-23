---
title: alpha
description: enable alpha blending
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: turn alpha blending on/off
    default: 1
  - type: float
    description: blending function
    default: GL_ONE_MINUS_SRC_ALPHA
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: blending function
outlets:
  1st:
    - type: gemlist
methods:
  - type: auto <float>
    description: turn on/off automatic depth detection
draft: false
---
