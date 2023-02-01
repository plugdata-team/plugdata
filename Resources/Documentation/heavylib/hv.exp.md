---
title: hv.exp

description: alternative implementation to the exp~ object

categories:
- object

pdcategory: heavylib, Signal Math

arguments:

inlets:
  1st:
  - type: signal
    description: input value to exp function

outlets:
  1st:
  - type: signal
    description: output of exp function

draft: false
---
This abstraction presents an alternative implementation to the exp~ object. Heavy uses the system's expf() function (as does Pd), but some applications may wish to avoid the expf() function entirely, and instead use this reasonably fast approximation.

