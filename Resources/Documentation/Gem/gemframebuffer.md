---
title: gemframebuffer
description: renders a scene in a texture for later use
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: type <symbol>
    description: type of the framebuffer data (BYTE, INT or FLOAT])
  - type: dimen <float> <float>
    description: dimension of the framebuffer texture
  - type: color <float> <float> <float> <float>
    description: background color of the framebuffer
  - type: texunit <float>
    description: change texunit of the texture
  - type: rectangle <float>
    description: texturing mode; rectangle (1) or normalized (0)
  - type: perspec <float> <float> <float> <float> <float> <float>
    description: frustum of the framebuffer
  - type: format <symbol>
    description: color format of the framebuffer (RGB|RGBA|RGB32|RGBA32F|YUV)
inlets:
  1st:
    - type: gemlist
outlets:
  1st:
    - type: gemlist
  2nd:
    - type: list
      description: texture info
draft: false
---
