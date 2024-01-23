---
title: scopeXYZ~
description: 3D oscilloscope
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: number of signal points to be stored
    default: blocksize
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: signal
      description: x-values of the oscilloscope
  3rd:
    - type: signal
      description: y-values of the oscilloscope
  4th:
    - type: signal
      description: z-values of the oscilloscope
methods:
  - type: draw [default|line|linestrip|fill|point|tri|tristrip|trifan|quad|quadstrip]
    description:
  - type: width <float>
    description: line-width (default: 1)
  - type: length <float>
    description: number of signal points to be stored
outlets:
  1st:
    - type: gemlist
draft: false
---
