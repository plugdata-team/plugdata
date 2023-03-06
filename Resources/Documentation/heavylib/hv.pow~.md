---
title: hv.pow~

description: alternative implementation to the pow~ object

categories:
- object

pdcategory: heavylib, Signal Math

arguments:

inlets:
  1st:
  - type: signal
    description: input value to power function
  2nd:
  - type: signal
    description: set numeric power to raise to

outlets:
  1st:
  - type: signal
    description: output of power function

draft: false
---
This abstraction presents an alternative implementation to the pow~ object. Heavy uses the system's powf() function (as does Pd), but some applications may wish to avoid the powf() function entirely. Both inlets are signal-rate. This abstraction may also make use of the hv.exp object instead of the exp~ object.

