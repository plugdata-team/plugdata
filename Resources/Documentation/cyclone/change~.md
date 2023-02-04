---
title: change~
description: detect signal change & direction
categories:
 - object
pdcategory: cyclone, Analysis
arguments:
inlets:
  1st:
  - type: signal
    description: signal to detect change & direction
  - type: float
    description: when DSP is off, sets an internal value to be compared with incoming signal when DSP is on
outlets:
  1st:
  - type: signal
    description: 1 when increasing, -1 when decreasing and 0 for no change

---

Use [change~] to detect if signal is changing and its direction. It outputs "1" if the signal is increasing, "-1" if it's decreasing and "0" if it doesn't change.

