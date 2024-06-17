---
title: multimodel
description: load multiple Alias/Wavefront models and render one of them
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: open <symbol> <float> <float> <float>
    description: load models with a specified file name pattern, base number, top number, and skip rate.
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
draft: false
---
