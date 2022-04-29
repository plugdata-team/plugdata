---
title: tabwrite~
description: write a signal in an array.
categories:
- object
see_also:
- tabread4~
- tabread
- tabwrite
- tabsend~
- tabreceive~
- soundfiler
pdcategory: Audio Oscillators And Tables
last_update: '0.40'
inlets:
  1st:
  - type: signal
    description: signal to write to an array.
  - type: start <float>
    description: starts recording at given sample 
  default: 0
  - type: bang
    description: starts recording into the array (same as 'start 0'