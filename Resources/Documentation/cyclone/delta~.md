---
title: delta~
description: difference between samples
categories:
 - object
pdcategory: cyclone, Data Math
arguments:
inlets:
  1st:
  - type: signal
    description: input signal to be evaluated
outlets:
  1st:
  - type: signal
    description: difference between the current and last sample

---

[delta~] outputs the difference between each incoming sample and the previous sample. So, if the input signal contains 1, 0.5, 2, 0.5, the output would be 1, -0.5, 1.5, -1.5

