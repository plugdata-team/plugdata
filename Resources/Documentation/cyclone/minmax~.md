---
title: minmax~
description: minimum/Maximum values of a signal
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
inlets:
  1st:
  - type: signal
    description: an input signal to analyze
  - type: bang
    description: outputs minimum and maximum on float outlets
  2nd:
  - type: signal
    description: a non-0 value resets minimum and maximum
outlets:
  1st:
  - type: signal
    description: minimum level since startup or last reset
  2nd:
  - type: signal
    description: maximum level since startup or last reset
  3rd:
  - type: float
    description: on bang: minimum level since startup or last reset
  4th:
  - type: float
    description: on bang: maximum level since startup or last reset

methods:
  - type: reset
    description: resets minimum and maximum (to current input value)

---

[minmax~] outputs the minimum and maximum values (as signals and floats) of an input signal since the startup or a reset.

