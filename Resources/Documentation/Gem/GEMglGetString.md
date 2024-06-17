
---
title: GEMglGetString
description: return a string describing the current GL connection
categories:
  - object
pdcategory: Gem, Graphics
arguments:
    - type: float
      description: Specifies the information to return.
      default: GL_VENDOR
inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: Specifies the information to return.
  3rd:
    - type: gemlist
      description: Returns a string describing the current GL connection.
outlets:
  1st:
    - type: gemlist
draft: false
---

