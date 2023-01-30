---
title: cmul~

description: complex multiplication

categories:
 - object

pdcategory: ELSE, Signal Math

arguments:
inlets:
  1st:
  - type: signal
    description: real part of complex number 1
  2nd:
  - type: signal
    description: imaginary part of complex number 1
  3rd:
  - type: signal
    description: real part of complex number 2
  4th:
  - type: signal
    description: imaginary part of complex number 1

outlets:
  1st:
  - type: signal
    description: real part of the multiplication result
  2nd:
  - type: signal
    description: imaginary part of the multiplication result

draft: false
---

[cmul~] performs complex multiplication of two complex numbers. It's a simple object used in the [conv~] abstraction in ELSE for a bit more efficiency.
