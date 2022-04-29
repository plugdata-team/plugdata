---
title: tabsend~
description: write a block of a signal to an array continuously
categories:
- object
see_also:
- send~
- block~
- array
- tabwrite~
- tabreceive~
pdcategory: Audio Oscillators And Tables
last_update: '0.43'
inlets:
  1st:
  - type: signal
    description: signal to send to matching tabreceive~ object(s).
  - type: set <name>
    description: set table name.
arguments:
  - type: symbol
    description: send symbol name (default empty symbol).
draft: false
---
By default a block is 64 samples but this can be changed with the block~ object.