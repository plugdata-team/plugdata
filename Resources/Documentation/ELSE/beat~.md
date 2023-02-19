---
title: beat~

description: beat detection

categories:
 - object

pdcategory: ELSE, Analysis

arguments:
  - type: float
    description: level threshold (default 0)

inlets:
  1st:
  - type: signal
    description: input to analyze
  2nd:
  - type: float
    description: level threshold

outlets:
  1st:
  - type: bang
    description: at beat detections

draft: false
---

[beat~] takes an input signal and outputs bangs at each beat detection. The argument or right inlet set the threshold of the onset detection.
