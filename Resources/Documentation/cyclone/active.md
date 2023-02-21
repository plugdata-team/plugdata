---
title: active
description: report window activity
categories:
 - object
pdcategory: cyclone, UI
arguments:
inlets:
outlets:
  1st:
  - type: float
    description: window status (0 inactive / 1 active)

draft: false
---

[active] outputs 1 when a patch canvas becomes active (its title bar is highlighted and it's the front-most window), and outputs 0 when inactive. See examples below.

