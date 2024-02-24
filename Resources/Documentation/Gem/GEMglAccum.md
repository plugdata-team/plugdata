
---
title: GEMglAccum
description: perform pixel arithmetic on the accumulation buffer
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the operation to be performed.
      default: 0
    - type: float
      description: Specifies the value to be used in the operation.
      default: 0
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the operation to be performed.
  3rd:
    - type: float
      description: Specifies the value to be used in the operation.
outlets:
  1st:
    - type: gemlist
draft: false
---

