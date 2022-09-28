---
title: cmul~

description: Complex multiplication

categories:
 - object

pdcategory: Math Functions

arguments: none

inlets:
  1st:
  - type: signal
    description: real part of complex number 1
  2nd:
  - type: signal
    description: imaginary part of complex nunmber 1
  3rd:
  - type: signal
    description: real part of complex number 2
  4th:
  - type: signal
    description: imaginary part of complex nunmber 1

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