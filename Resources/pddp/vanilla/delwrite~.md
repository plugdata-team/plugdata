---
title: delwrite~
description: write in a delay line.
categories:
- object
see_also:
- fexpr~
- delread~
- delread4~
pdcategory: Audio Delay
last_update: '0.52'
inlets:
  1st:
  - type: signal
    description: signal input into the delay time.
  - type: clear
    description: clear the delay time.
arguments:
  - type: symbol
    description: delay line name.
  - type: float
    description: length of delay line in msec (the maximum delay time in read objects).
draft: false
---
Delread~ and delread4~ objects read from a delay line allocated in a delwrite~ object with the same name. Note that in this help file we're using delay names with "$0" (the patch ID number used to force locality in Pd). You can use more than one delread~ and/or delread4~ objects for the same delwrite~ object. If the specified delay time in delread~/delread4~ is longer than the size of the delay line or less than zero it is clipped to the length of the delay line.

In case the delwrite~ runs later in the DSP loop than the delread~ or delread4~ objects, the delay is constrained below by one vector length (usually 64 samples).
