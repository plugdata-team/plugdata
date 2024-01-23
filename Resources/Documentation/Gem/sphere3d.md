---
title: sphere3d
description: renders a segmented sphere3d.
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: draw [line|fill|point]
  - type: float
    description: size of the sphere3d
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: size
  3rd:
    - type: float
      description: number of segments
outlets:
  1st:
    - type: gemlist
      description:
methods:
  - type: setCartesian <float> <float> <float> <float> <float>
    description: Set Cartesian coordinates for a specific point.
  - type: setSpherical <float> <float> <float> <float> <float>
    description: Set Spherical coordinates for a specific point (in degrees).
  - type: print
    description: Print information.
draft: false
---
