---
title: avg~
description: signal absolute average
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
inlets:
  1st:
  - type: signal
    description: the signal to take its absolute average
  - type: bang
    description: compute average since the last received bang
outlets:
  1st:
  - type: float
    description: the input signal's absolute average

---

Use the [avg~] object to keep track of the absolute average from the input signal when receiving a bang. The output value is the sum of the absolute values of the input divided by the number of elapsed samples.

