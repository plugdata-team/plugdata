---
title: canvas.edit

description: report edit status

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth
  default: 0 - current window

inlets:
  1st:
  - type: bang
    description: query for edit status

outlets:
  1st:
  - type: float
    description: edit status

draft: false
---

The [canvas.edit] object outputs edit status - 1 when the window is in edit mode and 0 when it is in run mode.
