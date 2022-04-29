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
    description: starts recording at given sample (default 0).
  - type: bang
    description: starts recording into the array (same as 'start 0').
  - type: stop
    description: stops recording into the array.
  - type: set <symbol>
    description: set the table name.
arguments:
  - type: symbol
    description: sets table name with the sample.
draft: false
---
Tabwrite~ records an audio signal sequentially into an array. Sending it "bang" writes from beginning to end of the array. To avoid writing all the way to the end, you can send a "stop" message at an appropriate later time. The "start" message allows setting the array location at which the first sample is written. (Starting and stopping occur on block boundaries, typically multiples of 64 samples, in the input signal.)