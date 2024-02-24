
---
title: GEMglStencilFunc
description: set front and back function and reference value for stencil testing
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the stencil test function.
    - type: float
      description: Specifies the reference value for the stencil test.
    - type: float
      description: Specifies a mask that is ANDed with both the reference value and the stored stencil value when the test is done.
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the stencil test function.
  3rd:
    - type: float
      description: Specifies the reference value for the stencil test.
  4th:
    - type: float
      description: Specifies a mask that is ANDed with both the reference value and the stored stencil value when the test is done.
outlets:
  1st:
    - type: gemlist
draft: false
---

