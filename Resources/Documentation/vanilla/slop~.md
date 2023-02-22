---
title: slop~
description: slew-limiting lowpass filter
categories:
- object
see_also:
- lop~
- fexpr~
pdcategory: vanilla, Filters, Effects
last_update: '0.50'

arguments:
  - type: float
    description: cutoff frequency in linear region 
    default: 0
  - type: float
    description: maximum downward slew of linear region 
    default: 0
  - type: float
    description: asymptotic downward cutoff frequency
    default: 0
  - type: float
    description: maximum upward slew of linear region
    default: 0
  - type: float
    description: asymptotic upward cutoff frequency
    default: 0
inlets:
  1st:
  - type: signal
    description: input signal to be filtered
  2nd:
  - type: signal
    description: cutoff frequency in linear region
  3rd:
  - type: signal
    description: maximum downward slew of linear region
  4th:
  - type: signal
    description: asymptotic downward cutoff frequency
  5th:
  - type: signal
    description: maximum upward slew of linear region
  6th:
  - type: signal
    description: asymptotic upward cutoff frequency
outlets:
  1st:
  - type: signal
    description: filtered signal

methods:
  - type: set <float>
    description: set state (previously stored output)
draft: false
---
