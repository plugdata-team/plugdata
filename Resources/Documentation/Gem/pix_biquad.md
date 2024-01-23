---
title: pix_biquad
description: time-based IIR filter
categories:
  - object
pdcategory: Gem, Graphics
methods:
  - type: set
    description: overwrites the filter-buffers with the next incoming image
inlets:
  1st:
    - type: gemlist
    - type: <list>
      description: the filter-coefficients "fb0 fb1 fb2 ff1 ff2 ff3"
outlets:
  1st:
    - type: gemlist
draft: false
---
