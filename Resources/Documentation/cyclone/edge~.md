---
title: edge~
description: detect signal transitions
categories:
 - object
pdcategory: cyclone, Analysis
arguments:
inlets:
  1st:
  - type: signal
    description: the signal to analyse
outlets:
  1st:
  - type: bang
    description: at zero to non-zero transition
  2nd:
  - type: bang
    description: at non-zero to zero transition

---

[edge~] detects signal transitions from zero to non-zero and vice versa and reports bangs accordingly.

