---
title: pix_buf
description: buffer a pix
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: the default value to force processing
methods:
  - type: auto <float>
    description: force image-processing in subsequent objects each render-cycle
inlets:
  1st:
    - type: gemlist
    - type: bang
      description: copy of input-data to the output and force all subsequent [pix_]-objects to process
outlets:
  1st:
    - type: gemlist
draft: false
---
