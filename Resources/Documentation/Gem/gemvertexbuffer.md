---
title: gemvertexbuffer
description: renders a vertex buffer
categories:
  - object
pdcategory: Gem, Graphics
arguments: null
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
      description:
methods:
  - type: position / posX / posY / posZ
    description: update vertex positions from tables
  - type: color / colorR / colorG / colorB / colorA
    description: update vertex colors from tables
  - type: normal / normalX / normalY / normalZ
    description: update vertex normals from tables
  - type: texture / textureU / textureV
    description: update vertex texcoords from tables
  - type: resize <float>
    description: change the number of vertices to use
  - type: draw_range <float> <float>
    description: set the range for partial draw
  - type: program <float>
    description: set the ID for GLSL program.
  - type: position_enable <float>, color_enable <float>, texture_enable <float>, normal_enable <float>, attribute_enable <float>
    description: enable/disable the use of position, color, texture, normal, and attribute data
  - type: attribute <name> <table> (offset)
    description: add attribute/update attribute from table
  - type: reset_attributes
    description: clear attribute data
  - type: print_attributes
    description: print active attributes
draft: false
---
