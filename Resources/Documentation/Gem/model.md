---
title: model
description: renders an Alias/Wavefront model
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: <symbol>
    description: name of an OBJ-file to be loaded
methods:
  - type: open <symbol>
    description: load an obj-file
  - type: rescale <float>
    description: normalize the model (must be set PRIOR to opening a model)
  - type: material<float>
    description: use material-information (from the mtl file)
  - type: group <float>
    description: draw only the specified part of the model (0 == all groups)
  - type: revert <float>
    description: revert faces
  - type: smooth <float>
    description: set smoothing factor (0.0 == flat, 1.0 == smooth)
  - type: texture <float>
    description: 0 == linear texturing, 1 == sphere mapping, 2 == UV-mapping
  - type: loader <symbol>
    description: choose which backend to use first to open the model
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
draft: false
---
