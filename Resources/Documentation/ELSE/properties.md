---
title: properties
description: respond to properties on parent

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: sets mode, non-0 is "subpatch" mode
  default: 0 ("abstraction" mode)

inlets:

outlets:
  1st:
  - type: bang
    description: bang when asked for properties in the parent patch

draft: false
---

[properties] sends a bang when you put it in an abstraction or a subpatch and ask for properties on the parent patch (by selecting the "properties" option after right clicking). This is useful for making properties for graphical subpatches or abstractions.

