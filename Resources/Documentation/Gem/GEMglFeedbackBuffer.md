
---
title: GEMglFeedbackBuffer
description: returns information about primitive groups in the feedback buffer
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the size of the buffer.
      default: 0
    - type: float
      description: Specifies the type of data to be returned.
      default: GL_3D_COLOR
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the size of the buffer.
  3rd:
    - type: float
      description: Specifies the type of data to be returned.
outlets:
  1st:
    - type: gemlist
draft: false
---

