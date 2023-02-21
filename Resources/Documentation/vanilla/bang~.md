---
title: bang~
description: output bang after each DSP cycle
categories:
- object
see_also:
- block~
pdcategory: vanilla, Triggers and Clocks
last_update: '0.33'
inlets:
  1st:
  - type: null
    description: inactive inlet
outlets:
  1st:
  - type: bang
    description: at every DSP block cycle when DSP is on
draft: false
---
bang~ outputs a bang after each DSP block cycle (at the same logical time as the DSP cycle.) This is primarily useful for sampling the outputs of analysis algorithms.

By default, a block size is 64 samples, at a 44100 sample rate, this about 1.45 ms. You can change the sample rate in audio settings and the block size with the block~ or switch~ object. Note that the minimum block size bang~ can handle is 64!
