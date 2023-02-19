---
title: fontsize
description: report font size

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth
  default: 0

inlets:
  1st:
  - type: bang
    description: report font size

outlets:
  1st:
  - type: float
    description: font size

draft: false
---

The [fontsize] object outputs the patch's font size when the object is loaded and when it changes via the font menu. You can also query with a bang message.

