---
title: canvas.zoom

description: report zoom status

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
    description: report zoom status

outlets:
  1st:
  - type: float
    description: zoom status

draft: false
---

The [canvas.zoom] object outputs zoom status at load time and when it changes - it outputs 1 when the window is in zoom mode and 0 when it is not. You can also query with a bang.
