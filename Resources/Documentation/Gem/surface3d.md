---
title: surface3d
description: Renders a 3d bicubic curve.
categories:
- object
pdcategory: Gem, Graphics
arguments:
- type: list
  description: size of the control matrix
  default: 4 4
methods:
- type: res <float> <float>
  description: Change the subdivision of the displayed curve.
- type: grid <float> <float>
  description: Change the size of the control matrix.
- type: set <float> <float> <float> <float>
  description: Set the position of a control point.
- type: normal <float>
  description: Enable/disable normals calculation
- type: draw <symbol>
  description: line, fill or point

inlets:
- 1st:
    - type: gemlist
      description:
  2nd:
    - type: unused
      description: unused

outlets:
- 1st:
    - type: gemlist
draft: false
---