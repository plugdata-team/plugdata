---
title: cusp~

description: Cusp map chaotic generator

categories:
 - object

pdcategory: Noise

arguments:
  1st:
  - type: float
    description: sets frequency in hertz
    default: nyquist
  2nd:
  - type: float
    description: sets 'a'
    default: 1
  1st:
  - type: float
    description: sets 'b'
    default: 0.19
  1st:
  - type: float
    description: sets initial value of y[n-1]
    default: 0

inlets:
  1st:
  - type: float/signal
    description: frequency in herz (negative values accepted)
  - type: list
    description: 3 floats sets 'a', 'b' and y[n-1]

outlets:
  1st:
  - type: signal
    description: cusp map chaotic signal

draft: false
---

[cusp~] is a chaotic generator using the difference equation;
y[n] = a - b * sqrt(abs(y[n-1]))
The output rate of the equation is given in hertz (default: nyquist).
Object based on SuperCollider's "CuspN" UGEN.