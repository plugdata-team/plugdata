---
title: edge~
description: Detect signal transitions
categories:
 - object
pdcategory: General
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

