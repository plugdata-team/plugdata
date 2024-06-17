
---
title: GEMglStencilOp
description: set front and back stencil test actions
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the action to take when the stencil test fails.
    - type: float
      description: Specifies the action to take when the stencil test passes, but the depth test fails.
    - type: float
      description: Specifies the action to take when both the stencil test and the depth test pass.
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the action to take when the stencil test fails.
  3rd:
    - type: float
      description: Specifies the action to take when the stencil test passes, but the depth test fails.
  4th:
    - type: float
      description: Specifies the action to take when both the stencil test and the depth test pass.
outlets:
  1st:
    - type: gemlist
draft: false
---

