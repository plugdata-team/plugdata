---
title: pix_freeframe
description: run a FreeFrame effect
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: the plugin to load
methods:
  - type: open
    description: load another plugin
inlets:
  1st:
    - type: gemlist
      description:
    - type: list <float> <float>
      description: set parameter arg1 to value of arg2
  nth:
    - type: <type>
      description: depending on the settable parameter
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
