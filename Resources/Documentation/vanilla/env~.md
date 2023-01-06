---
title: env~
description: envelope follower
categories:
- object
see_also: {}
pdcategory: Audio Filters
last_update: '0.40'
inlets:
  1st:
  - type: signal
    description: signal to be analyzed. 
outlets:
  1st:
  - type: float
    description: RMS envelope in dB. 
arguments:
  - type: float
    description: window size in samples 
  default: 1024
  - type: float
    description: period in samples per analysis 