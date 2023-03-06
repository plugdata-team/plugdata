---
title: cputime
description: measure CPU time
categories:
- object
pdcategory: vanilla, Triggers and Clocks, Analysis
last_update: '0.33'
see_also:
- realtime
- timer
inlets:
  1st:
  - type: bang
    description: reset (set elapsed time to zero)
  2nd:
  - type: bang
    description: time to measure
outlets:
  1st:
  - type: bang
    description: output elapsed time
draft: false
---
The cputime object measures elapsed CPU time,  as measured by your operating system. This appears to work on NT,  IRIX,  and Linux,  but not on W98.
