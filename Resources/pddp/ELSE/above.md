---
title: above

description: Threshold detection

categories:
- object

pdcategory: Math

arguments:
- description: initial threshold value 
  type: float
  default: 0

inlets:
  1st:
  - type: float
    description: input float 
  2nd:
  - type: float
    description: threshold value

outlets:
  1st:
  - type: bang
    description: when left inlet value rises above the threshold value
  2nd:
  - type: bang
    description: when left inlet falls back or below a threshold

draft: false
---

[above] sends a bang to the left outlet when the left inlet is above the threshold value. Conversely, it sends a bang to the right outlet when the left inlet falls back to the threshold value or below it. The threshold value can be set via the argument or the 2nd inlet.