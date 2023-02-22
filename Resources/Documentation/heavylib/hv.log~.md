---
title: hv.log~

description: signal rate logarithm operation

categories:
- object

pdcategory: heavylib, Signal Math

arguments:

inlets:
  1st:
  - type: signal
    description: input to log function
  2nd:
  - type: float
    description: base (default e)

outlets:
  1st:
  - type: signal
    description: output of the log function

draft: false
---
Signal rate logarithm operation. The left inlet must be a signal and the right inlet takes the base. The base may optionally also be initialised with the object. If no base is supplied, then the natural logarithm is assumed (base e).

