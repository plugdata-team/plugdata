---
title: pix_texture
description: apply texture mapping
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: env <float>
    description: texture environment mode (0=GL_REPLACE, 1=GL_DECAL, 2=GL_BLEND, 3=GL_ADD, 4=GL_COMBINE, >4=GL_MODULATE)
  - type: quality <float>
    description: 0=GL_NEAREST, 1=GL_LINEAR
  - type: repeat <float>
    description: 0=CLAMP_TO_EDGE, 1=REPEAT
  - type: rectangle <float>
    description: use rectangle-texturing if available
  - type: client_storage <float>
    description: use client-storage if available
  - type: env <float>
    description: texture environment mode
  - type: texunit <float>
    description: useful only with shader, change texunit of the texture
  - type: pbo
    description: change pixel buffer object number
  - type: yuv
    description: use native YUV-mode if available
inlets:
  1st:
    - type: gemlist
      description:
    - type: float
      description: turn texturing on/off

outlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: texture info <id> <width> <height> <type> <upsidedown flag>

draft: false
---
