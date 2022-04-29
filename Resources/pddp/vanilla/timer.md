---
title: timer
description: measure time intervals
categories:
- object
pdcategory: Time
last_update: '0.47'
see_also:
- cputime
- realtime
- delay
- metro
- text sequence
arguments:
- description: tempo value (default 1).
  type: float
- description: time unit (default 'msec').
  type: symbol
inlets:
  1st:
  - type: bang
    description: reset (set elapsed time to zero).
  - type: tempo <float,  symbol>
    description: set tempo value (float) and time unit symbol.
  2nd:
  - type: bang
    description: time to measure.
outlets:
  1st:
  - type: bang
    description: output elapsed time.
draft: false
---
Measure logical time.

The timer object measures elapsed logical time. Logical time moves forward as if all computation were instantaneous and as if all "delay" and "metro" objects were exact. By default,  the time unit is 1 millisecond,  but you can change this with the 'tempo' message (as in [delay],  [metro] and [text sequence]),  which takes a different tempo number and a time unit symbol. Possible symbols are:

- millisecond (msec for short)
- seconds (sec)
- minutes (min)
- samples (samp) - depends on the sample rate the patch is running

These symbols can also be preceded by "per" (as in "permin",  "permsec",  etc.) In this case,  60 permin means 1/60 min (hence,  the same as 'BPM').

Note you need to reset the elapsed time to zero when you change the tempo message when the object is running,  otherwise you get funny results because the change takes effect immediately and gets applied to the remaining part of the elapsed time.
